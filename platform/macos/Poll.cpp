#include "platform/Poll.h"
#include "platform/Socket.h"

#include <fcntl.h>
#include <sys/event.h>
#include <unistd.h>

#include <stdexcept>
#include <vector>

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

Poller::Poller() : impl_(std::make_unique<Impl>()) {
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
    if (n > 0)
        kevent(impl_->kq, changes, n, nullptr, 0, nullptr);
}

void Poller::modify(int fd, uint32_t interest, void* user_data) {
    struct kevent changes[4];
    int n = 0;
    EV_SET(&changes[n++], fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
    EV_SET(&changes[n++], fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
    kevent(impl_->kq, changes, n, nullptr, 0, nullptr);

    n = 0;
    if (interest & Readable)
        EV_SET(&changes[n++], fd, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, user_data);
    if (interest & Writable)
        EV_SET(&changes[n++], fd, EVFILT_WRITE, EV_ADD | EV_CLEAR, 0, 0, user_data);
    if (n > 0)
        kevent(impl_->kq, changes, n, nullptr, 0, nullptr);
}

void Poller::remove(int fd) {
    struct kevent changes[2];
    EV_SET(&changes[0], fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
    EV_SET(&changes[1], fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
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
        out[i] = Event{static_cast<int>(ke.ident), flags, ke.udata};
    }
    return n;
}

// -- notifier ---------------------------------------------------------------

int Poller::create_notifier() {
    if (::pipe(impl_->notify_pipe) < 0)
        throw std::runtime_error("pipe() failed for notifier");

    // Make read end non-blocking
    int flags = fcntl(impl_->notify_pipe[0], F_GETFL, 0);
    fcntl(impl_->notify_pipe[0], F_SETFL, flags | O_NONBLOCK);

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
