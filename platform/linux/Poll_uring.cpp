/**
 * @file Poll_uring.cpp
 * @brief io_uring-based Poller implementation for Linux.
 *
 * Completion-mode backend: recv() is performed by the kernel and delivered
 * as completed buffers in Event::data/data_len.  The poll thread no longer
 * needs to call recv() — it gets the data directly from the completion ring.
 *
 * Requires Linux 5.7+ and liburing.
 * Build with -DAO_USE_IO_URING=ON (default on Linux).
 */
#include "platform/Poll.h"
#include "platform/Socket.h"

#include <liburing.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "utils/Log.h"

namespace platform {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr size_t BUF_SIZE = 8192;        // bytes per buffer
static constexpr uint16_t BUF_GROUP_ID = 0;

// Tags encoded in sqe->user_data to distinguish completion types.
// The low 2 bits encode the operation type; the upper bits carry the fd.
enum OpTag : uint8_t {
    OP_POLL = 0,  // poll_add (readiness — for listener, notifier, writable)
    OP_RECV = 1,  // recv with provided buffer
    OP_SEND = 2,  // send (async write)
    OP_NOP = 3,   // internal (e.g. cancelled)
};

static uint64_t make_tag(int fd, OpTag op) {
    return (static_cast<uint64_t>(fd) << 2) | op;
}
static int tag_fd(uint64_t tag) {
    return static_cast<int>(tag >> 2);
}
static OpTag tag_op(uint64_t tag) {
    return static_cast<OpTag>(tag & 0x3);
}

// ---------------------------------------------------------------------------
// Impl
// ---------------------------------------------------------------------------

struct Poller::Impl {
    struct io_uring ring{};
    int efd = -1; // eventfd for cross-thread notification

    // Provided buffer pool
    unsigned buf_count = 0;       // number of buffers (power of two)
    struct io_uring_buf_ring* buf_ring = nullptr;
    uint8_t* buf_pool = nullptr;  // contiguous allocation: buf_count * BUF_SIZE

    // Per-fd state
    struct FdState {
        uint32_t interest = 0;
        void* user_data = nullptr;
        bool recv_pending = false; // true if an OP_RECV is in flight
        // Async send state
        const void* send_buf = nullptr;
        size_t send_len = 0;
        size_t send_offset = 0;
    };
    std::unordered_map<int, FdState> fds;

    explicit Impl(unsigned io_buffers) : buf_count(io_buffers) {
        // Ring entries should be at least as large as the buffer count
        // so we can have one SQE per buffer in flight.
        unsigned ring_entries = std::max(io_buffers, 256u);

        struct io_uring_params params{};
        int ret = io_uring_queue_init_params(ring_entries, &ring, &params);
        if (ret < 0)
            throw std::runtime_error(std::string("io_uring_queue_init: ") + strerror(-ret));

        // Set up provided buffer ring
        buf_pool = new uint8_t[buf_count * BUF_SIZE];
        buf_ring = io_uring_setup_buf_ring(&ring, buf_count, BUF_GROUP_ID, 0, &ret);
        if (!buf_ring) {
            delete[] buf_pool;
            io_uring_queue_exit(&ring);
            throw std::runtime_error(std::string("io_uring_setup_buf_ring: ") + strerror(-ret));
        }

        // Register all buffers in the ring
        for (unsigned i = 0; i < buf_count; ++i) {
            io_uring_buf_ring_add(buf_ring, buf_pool + i * BUF_SIZE, BUF_SIZE,
                                  static_cast<uint16_t>(i),
                                  io_uring_buf_ring_mask(buf_count), i);
        }
        io_uring_buf_ring_advance(buf_ring, buf_count);
    }

    ~Impl() {
        if (efd >= 0)
            ::close(efd);
        delete[] buf_pool;
        io_uring_queue_exit(&ring);
    }

    // Diagnostic counters
    uint64_t stat_recv_submitted = 0;
    uint64_t stat_recv_completed = 0;
    uint64_t stat_recv_enobufs = 0;
    uint64_t stat_send_submitted = 0;
    uint64_t stat_send_completed = 0;
    uint64_t stat_send_partial = 0;
    uint64_t stat_send_errors = 0;
    uint64_t stat_sqe_full = 0;
    uint64_t stat_cqe_reaped = 0;
    uint64_t stat_poll_cycles = 0;

    /// Submit a recv using a provided buffer for the given fd.
    void submit_recv(int fd) {
        auto it = fds.find(fd);
        if (it == fds.end() || it->second.recv_pending)
            return;

        struct io_uring_sqe* sqe = io_uring_get_sqe(&ring);
        if (!sqe) {
            ++stat_sqe_full;
            return;
        }

        io_uring_prep_recv(sqe, fd, nullptr, BUF_SIZE, 0);
        sqe->flags |= IOSQE_BUFFER_SELECT;
        sqe->buf_group = BUF_GROUP_ID;
        io_uring_sqe_set_data64(sqe, make_tag(fd, OP_RECV));
        it->second.recv_pending = true;
        ++stat_recv_submitted;
    }

    /// Submit a poll_add for readiness notification (used for listener,
    /// notifier, and Writable interest).
    void submit_poll(int fd, uint32_t interest) {
        struct io_uring_sqe* sqe = io_uring_get_sqe(&ring);
        if (!sqe) {
            ++stat_sqe_full;
            return;
        }

        unsigned poll_mask = 0;
        if (interest & Poller::Readable)
            poll_mask |= POLLIN;
        if (interest & Poller::Writable)
            poll_mask |= POLLOUT;

        io_uring_prep_poll_add(sqe, fd, poll_mask);
        io_uring_sqe_set_data64(sqe, make_tag(fd, OP_POLL));
    }

    /// Submit an async send for the given fd.
    void do_submit_send(int fd) {
        auto it = fds.find(fd);
        if (it == fds.end() || !it->second.send_buf)
            return;

        auto& s = it->second;
        struct io_uring_sqe* sqe = io_uring_get_sqe(&ring);
        if (!sqe) {
            ++stat_sqe_full;
            Log::warn("io_uring: SQE ring full on send for fd {}", fd);
            return;
        }
        ++stat_send_submitted;

        io_uring_prep_send(sqe, fd,
                           static_cast<const char*>(s.send_buf) + s.send_offset,
                           s.send_len - s.send_offset, MSG_NOSIGNAL);
        io_uring_sqe_set_data64(sqe, make_tag(fd, OP_SEND));
    }
};

// ---------------------------------------------------------------------------
// Poller lifecycle
// ---------------------------------------------------------------------------

Poller::Poller(unsigned io_buffers) : impl_(std::make_unique<Impl>(io_buffers)) {
}
Poller::~Poller() = default;
Poller::Poller(Poller&&) noexcept = default;
Poller& Poller::operator=(Poller&&) noexcept = default;

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

/// Check if fd is a listening socket (needs accept, not recv).
static bool is_listener(int fd) {
    int val = 0;
    socklen_t len = sizeof(val);
    if (getsockopt(fd, SOL_SOCKET, SO_ACCEPTCONN, &val, &len) == 0)
        return val != 0;
    return false;
}

void Poller::add(int fd, uint32_t interest, void* user_data) {
    impl_->fds[fd] = {interest, user_data, false};

    // Decide mode:
    // - Notifier (eventfd): readiness (needs drain_notifier)
    // - Listener socket: readiness (needs accept)
    // - Regular connection: completion (kernel does recv, delivers buffer)
    bool use_readiness = (fd == impl_->efd) || is_listener(fd);

    if (use_readiness) {
        impl_->submit_poll(fd, interest);
    } else {
        if (interest & Readable)
            impl_->submit_recv(fd);
        if (interest & Writable)
            impl_->submit_poll(fd, Writable);
    }
    io_uring_submit(&impl_->ring);
}

void Poller::add(const Socket& sock, uint32_t interest, void* user_data) {
    add(sock.fd(), interest, user_data);
}

void Poller::modify(int fd, uint32_t interest, void* user_data) {
    auto it = impl_->fds.find(fd);
    if (it == impl_->fds.end()) {
        add(fd, interest, user_data);
        return;
    }
    it->second.interest = interest;
    it->second.user_data = user_data;
    // Re-submit if needed
    if ((interest & Readable) && !it->second.recv_pending && fd != impl_->efd)
        impl_->submit_recv(fd);
    io_uring_submit(&impl_->ring);
}

void Poller::modify(const Socket& sock, uint32_t interest, void* user_data) {
    modify(sock.fd(), interest, user_data);
}

void Poller::remove(int fd) {
    // Cancel any in-flight operations for this fd
    struct io_uring_sqe* sqe = io_uring_get_sqe(&impl_->ring);
    if (sqe) {
        io_uring_prep_cancel_fd(sqe, fd, 0);
        io_uring_sqe_set_data64(sqe, make_tag(fd, OP_NOP));
        io_uring_submit(&impl_->ring);
    }
    impl_->fds.erase(fd);
}

void Poller::remove(const Socket& sock) {
    remove(sock.fd());
}

// ---------------------------------------------------------------------------
// Poll — reap completions
// ---------------------------------------------------------------------------

int Poller::poll(Event* out, int max_events, int timeout_ms) {
    // Submit any pending SQEs before waiting
    io_uring_submit(&impl_->ring);

    // Wait for at least one completion
    struct __kernel_timespec ts{};
    struct __kernel_timespec* ts_ptr = nullptr;
    if (timeout_ms >= 0) {
        ts.tv_sec = timeout_ms / 1000;
        ts.tv_nsec = static_cast<long long>(timeout_ms % 1000) * 1000000LL;
        ts_ptr = &ts;
    }

    struct io_uring_cqe* cqe = nullptr;
    int ret;
    if (ts_ptr)
        ret = io_uring_wait_cqe_timeout(&impl_->ring, &cqe, ts_ptr);
    else
        ret = io_uring_wait_cqe(&impl_->ring, &cqe);

    if (ret < 0 && ret != -ETIME)
        return 0; // error or signal

    // Reap all available completions
    int count = 0;
    unsigned seen = 0;
    unsigned head;
    io_uring_for_each_cqe(&impl_->ring, head, cqe) {
        ++seen;
        ++impl_->stat_cqe_reaped;
        if (count >= max_events)
            break;

        uint64_t tag = io_uring_cqe_get_data64(cqe);
        int fd = tag_fd(tag);
        OpTag op = tag_op(tag);

        // Look up fd state
        auto fd_it = impl_->fds.find(fd);
        if (fd_it == impl_->fds.end()) {
            // fd was removed — just consume the CQE
            goto next;
        }

        if (op == OP_RECV) {
            fd_it->second.recv_pending = false;

            if (cqe->res > 0) {
                ++impl_->stat_recv_completed;
                uint16_t bid = static_cast<uint16_t>(cqe->flags >> IORING_CQE_BUFFER_SHIFT);
                out[count++] = Event{
                    fd,
                    Readable,
                    fd_it->second.user_data,
                    impl_->buf_pool + bid * BUF_SIZE,
                    static_cast<size_t>(cqe->res),
                    bid,
                };
                impl_->submit_recv(fd);
            } else if (cqe->res == 0) {
                out[count++] = Event{fd, HangUp, fd_it->second.user_data, nullptr, 0, 0};
            } else if (cqe->res == -ENOBUFS) {
                ++impl_->stat_recv_enobufs;
                Log::warn("io_uring: ENOBUFS on fd {} — buffer pool exhausted, falling back to poll", fd);
                impl_->submit_poll(fd, Readable);
            } else if (cqe->res != -ECANCELED) {
                Log::warn("io_uring: recv error on fd {}: {}", fd, strerror(-cqe->res));
                out[count++] = Event{fd, Error, fd_it->second.user_data, nullptr, 0, 0};
            }
        } else if (op == OP_SEND) {
            auto& s = fd_it->second;
            if (cqe->res > 0) {
                s.send_offset += static_cast<size_t>(cqe->res);
                if (s.send_offset < s.send_len) {
                    ++impl_->stat_send_partial;
                    impl_->do_submit_send(fd);
                } else {
                    ++impl_->stat_send_completed;
                    size_t total = s.send_offset;
                    s.send_buf = nullptr;
                    s.send_len = 0;
                    s.send_offset = 0;
                    out[count++] = Event{fd, SendDone, fd_it->second.user_data, nullptr, total, 0};
                }
            } else {
                ++impl_->stat_send_errors;
                Log::warn("io_uring: send error on fd {}: {} (sent {}/{})",
                          fd, strerror(-cqe->res), s.send_offset, s.send_len);
                size_t total = s.send_offset;
                s.send_buf = nullptr;
                s.send_len = 0;
                s.send_offset = 0;
                out[count++] = Event{fd, SendDone | Error, fd_it->second.user_data, nullptr, total, 0};
            }
        } else if (op == OP_POLL) {
            // Readiness event (listener, notifier, writable, or fallback)
            uint32_t flags = 0;
            if (cqe->res & POLLIN)
                flags |= Readable;
            if (cqe->res & POLLOUT)
                flags |= Writable;
            if (cqe->res & POLLERR)
                flags |= Error;
            if (cqe->res & (POLLHUP | POLLRDHUP))
                flags |= HangUp;

            if (flags)
                out[count++] = Event{fd, flags, fd_it->second.user_data, nullptr, 0, 0};

            // Re-arm poll for this fd (poll_add is one-shot)
            if (fd_it != impl_->fds.end())
                impl_->submit_poll(fd, fd_it->second.interest);
        }
        // OP_NOP: cancellation confirmations — ignore

    next:;
    }

    io_uring_cq_advance(&impl_->ring, seen);

    // Submit any re-armed operations
    if (count > 0)
        io_uring_submit(&impl_->ring);

    return count;
}

// ---------------------------------------------------------------------------
// Stats
// ---------------------------------------------------------------------------

Poller::IoStats Poller::io_stats() const {
    return {
        impl_->stat_recv_submitted, impl_->stat_recv_completed, impl_->stat_recv_enobufs,
        impl_->stat_send_submitted, impl_->stat_send_completed, impl_->stat_send_partial,
        impl_->stat_send_errors, impl_->stat_sqe_full, impl_->stat_cqe_reaped,
    };
}

// ---------------------------------------------------------------------------
// Async send
// ---------------------------------------------------------------------------

bool Poller::submit_send(int fd, const void* data, size_t len, size_t* /*bytes_sent*/) {
    auto it = impl_->fds.find(fd);
    if (it == impl_->fds.end())
        return true; // unknown fd — caller handles

    it->second.send_buf = data;
    it->second.send_len = len;
    it->second.send_offset = 0;
    impl_->do_submit_send(fd);
    io_uring_submit(&impl_->ring);
    return false; // async — completion via SendDone event
}

// ---------------------------------------------------------------------------
// Buffer recycling
// ---------------------------------------------------------------------------

void Poller::recycle_buffer(uint16_t buffer_id) {
    io_uring_buf_ring_add(impl_->buf_ring,
                          impl_->buf_pool + buffer_id * BUF_SIZE,
                          BUF_SIZE, buffer_id,
                          io_uring_buf_ring_mask(impl_->buf_count), 0);
    io_uring_buf_ring_advance(impl_->buf_ring, 1);
}

// ---------------------------------------------------------------------------
// Notifier
// ---------------------------------------------------------------------------

int Poller::create_notifier() {
    impl_->efd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (impl_->efd < 0)
        throw std::runtime_error("eventfd() failed");
    add(impl_->efd, Readable);
    return impl_->efd;
}

void Poller::notify() {
    if (impl_->efd >= 0) {
        uint64_t val = 1;
        (void)::write(impl_->efd, &val, sizeof(val));
    }
}

void Poller::drain_notifier() {
    if (impl_->efd >= 0) {
        uint64_t val;
        (void)::read(impl_->efd, &val, sizeof(val));
    }
}

} // namespace platform
