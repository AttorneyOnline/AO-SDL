#include "platform/Poll.h"
#include "platform/Socket.h"

#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <stdexcept>
#include <vector>

namespace platform {

struct Poller::Impl {
    int epfd = -1;
    int efd = -1; // eventfd for notifications

    Impl() : epfd(epoll_create1(EPOLL_CLOEXEC)) {
        if (epfd < 0)
            throw std::runtime_error("epoll_create1() failed");
    }

    ~Impl() {
        if (efd >= 0)
            ::close(efd);
        if (epfd >= 0)
            ::close(epfd);
    }
};

Poller::Poller() : impl_(std::make_unique<Impl>()) {
}
Poller::~Poller() = default;
Poller::Poller(Poller&&) noexcept = default;
Poller& Poller::operator=(Poller&&) noexcept = default;

static uint32_t to_epoll_events(uint32_t interest) {
    uint32_t ev = EPOLLET;
    if (interest & Poller::Readable)
        ev |= EPOLLIN;
    if (interest & Poller::Writable)
        ev |= EPOLLOUT;
    return ev;
}

// -- fd-based (core) --------------------------------------------------------

void Poller::add(int fd, uint32_t interest, void* user_data) {
    struct epoll_event ev{};
    ev.events = to_epoll_events(interest);
    ev.data.ptr = user_data;
    epoll_ctl(impl_->epfd, EPOLL_CTL_ADD, fd, &ev);
}

void Poller::modify(int fd, uint32_t interest, void* user_data) {
    struct epoll_event ev{};
    ev.events = to_epoll_events(interest);
    ev.data.ptr = user_data;
    epoll_ctl(impl_->epfd, EPOLL_CTL_MOD, fd, &ev);
}

void Poller::remove(int fd) {
    epoll_ctl(impl_->epfd, EPOLL_CTL_DEL, fd, nullptr);
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
    std::vector<struct epoll_event> events(static_cast<size_t>(max_events));
    int n = epoll_wait(impl_->epfd, events.data(), max_events, timeout_ms);
    if (n < 0)
        return 0;

    for (int i = 0; i < n; ++i) {
        auto& ep = events[static_cast<size_t>(i)];
        uint32_t flags = 0;
        if (ep.events & EPOLLIN)
            flags |= Readable;
        if (ep.events & EPOLLOUT)
            flags |= Writable;
        if (ep.events & EPOLLERR)
            flags |= Error;
        if (ep.events & (EPOLLHUP | EPOLLRDHUP))
            flags |= HangUp;
        out[i] = Event{-1, flags, ep.data.ptr};
    }
    return n;
}

// -- notifier ---------------------------------------------------------------

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
        ::write(impl_->efd, &val, sizeof(val));
    }
}

void Poller::drain_notifier() {
    if (impl_->efd >= 0) {
        uint64_t val;
        ::read(impl_->efd, &val, sizeof(val));
    }
}

} // namespace platform
