#include "platform/Poll.h"
#include "platform/Socket.h"

#include <fcntl.h>
#include <sys/event.h>
#include <unistd.h>

#include <cstring>
#include <stdexcept>
#include <vector>

#include "utils/Log.h"

namespace platform {

struct Poller::Impl {
    int kq = -1;
    int notify_pipe[2] = {-1, -1}; // [0] = read end (polled), [1] = write end (signaled)

    Impl() : kq(kqueue()) {
        if (kq < 0)
            throw std::runtime_error("kqueue() failed");
    }

    ~Impl() {
        if (notify_pipe[0] >= 0)
            ::close(notify_pipe[0]);
        if (notify_pipe[1] >= 0)
            ::close(notify_pipe[1]);
        if (kq >= 0)
            ::close(kq);
    }
};

Poller::Poller(unsigned /*io_buffers*/) : impl_(std::make_unique<Impl>()) {
}
Poller::~Poller() = default;
Poller::Poller(Poller&&) noexcept = default;
Poller& Poller::operator=(Poller&&) noexcept = default;

// -- fd-based (core) --------------------------------------------------------

void Poller::add(int fd, uint32_t interest, void* user_data) {
    struct kevent changes[2];
    int n = 0;
    if (interest & Readable)
        EV_SET(&changes[n++], fd, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, user_data);
    if (interest & Writable)
        EV_SET(&changes[n++], fd, EVFILT_WRITE, EV_ADD | EV_CLEAR, 0, 0, user_data);
    if (n > 0) {
        if (kevent(impl_->kq, changes, n, nullptr, 0, nullptr) < 0)
            Log::warn("kevent ADD failed for fd {}: {}", fd, strerror(errno));
    }
}

void Poller::modify(int fd, uint32_t interest, void* user_data) {
    struct kevent changes[4];
    int n = 0;
    EV_SET(&changes[n++], fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
    EV_SET(&changes[n++], fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
    // Ignore errors on delete — filter may not exist
    kevent(impl_->kq, changes, n, nullptr, 0, nullptr);

    n = 0;
    if (interest & Readable)
        EV_SET(&changes[n++], fd, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, user_data);
    if (interest & Writable)
        EV_SET(&changes[n++], fd, EVFILT_WRITE, EV_ADD | EV_CLEAR, 0, 0, user_data);
    if (n > 0) {
        if (kevent(impl_->kq, changes, n, nullptr, 0, nullptr) < 0)
            Log::warn("kevent MOD failed for fd {}: {}", fd, strerror(errno));
    }
}

void Poller::remove(int fd) {
    struct kevent changes[2];
    EV_SET(&changes[0], fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
    EV_SET(&changes[1], fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
    // Ignore errors — filter may not exist (e.g. only Readable was added)
    kevent(impl_->kq, changes, 2, nullptr, 0, nullptr);
}

// -- Socket-based (delegate) ------------------------------------------------

void Poller::add(const Socket& sock, uint32_t interest, void* user_data) {
    add(sock.fd(), interest, user_data);
}

void Poller::modify(const Socket& sock, uint32_t interest, void* user_data) {
    modify(sock.fd(), interest, user_data);
}

void Poller::remove(const Socket& sock) {
    remove(sock.fd());
}

// -- poll -------------------------------------------------------------------

int Poller::poll(Event* out, int max_events, int timeout_ms) {
    std::vector<struct kevent> kev(static_cast<size_t>(max_events));

    struct timespec ts;
    struct timespec* ts_ptr = nullptr;
    if (timeout_ms >= 0) {
        ts.tv_sec = timeout_ms / 1000;
        ts.tv_nsec = (timeout_ms % 1000) * 1000000L;
        ts_ptr = &ts;
    }

    int n = kevent(impl_->kq, nullptr, 0, kev.data(), max_events, ts_ptr);
    if (n < 0)
        return 0;

    for (int i = 0; i < n; ++i) {
        auto& ke = kev[static_cast<size_t>(i)];
        uint32_t flags = 0;
        if (ke.filter == EVFILT_READ)
            flags |= Readable;
        if (ke.filter == EVFILT_WRITE)
            flags |= Writable;
        if (ke.flags & EV_EOF)
            flags |= HangUp;
        if (ke.flags & EV_ERROR)
            flags |= Error;
        out[i] = Event{static_cast<int>(ke.ident), flags, ke.udata, nullptr, 0, 0};
    }
    return n;
}

// -- buffer recycling (no-op for kqueue) ------------------------------------

void Poller::recycle_buffer(uint16_t /*buffer_id*/) {
}

Poller::IoStats Poller::io_stats() const {
    return {};
}

// -- send (no-op for kqueue — caller handles send inline) -------------------

bool Poller::submit_send(int /*fd*/, const void* /*data*/, size_t /*len*/, size_t* /*bytes_sent*/) {
    return true; // synchronous — caller must send inline
}

// -- notifier ---------------------------------------------------------------

int Poller::create_notifier() {
    if (::pipe(impl_->notify_pipe) < 0)
        throw std::runtime_error("pipe() failed for notifier");

    // Make read end non-blocking
    int flags = fcntl(impl_->notify_pipe[0], F_GETFL, 0);
    if (flags < 0 || fcntl(impl_->notify_pipe[0], F_SETFL, flags | O_NONBLOCK) < 0)
        Log::warn("Poller::create_notifier: fcntl O_NONBLOCK failed: {}", strerror(errno));

    add(impl_->notify_pipe[0], Readable);
    return impl_->notify_pipe[0];
}

void Poller::notify() {
    if (impl_->notify_pipe[1] >= 0) {
        char c = 1;
        ::write(impl_->notify_pipe[1], &c, 1);
    }
}

void Poller::drain_notifier() {
    if (impl_->notify_pipe[0] >= 0) {
        char buf[64];
        while (::read(impl_->notify_pipe[0], buf, sizeof(buf)) > 0) {
        }
    }
}

} // namespace platform
