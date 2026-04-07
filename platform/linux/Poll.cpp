#include "platform/Poll.h"
#include "platform/Socket.h"

#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <cstring>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "utils/Log.h"

namespace platform {

struct Poller::Impl {
    int epfd = -1;
    int efd = -1;                                 // eventfd for notifications
    std::unordered_map<int, void*> user_data_map; // fd → user_data (epoll can't store both)

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

Poller::Poller(unsigned /*io_buffers*/) : impl_(std::make_unique<Impl>()) {
}
Poller::~Poller() = default;
Poller::Poller(Poller&&) noexcept = default;
Poller& Poller::operator=(Poller&&) noexcept = default;

// Edge-triggered mode (EPOLLET): the kernel delivers an event only once per
// state transition, NOT every time data is available. Consumers MUST drain
// the socket completely (recv until EAGAIN) after each readable event,
// otherwise remaining data won't trigger another notification until *new*
// data arrives. The HttpServer poll loop and WebSocketServer both follow
// this pattern. If adding a new poller consumer, ensure it drains fully.
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
    ev.data.fd = fd;
    if (epoll_ctl(impl_->epfd, EPOLL_CTL_ADD, fd, &ev) < 0)
        Log::warn("epoll_ctl ADD failed for fd {}: {}", fd, strerror(errno));
    impl_->user_data_map[fd] = user_data;
}

void Poller::modify(int fd, uint32_t interest, void* user_data) {
    struct epoll_event ev{};
    ev.events = to_epoll_events(interest);
    ev.data.fd = fd;
    if (epoll_ctl(impl_->epfd, EPOLL_CTL_MOD, fd, &ev) < 0)
        Log::warn("epoll_ctl MOD failed for fd {}: {}", fd, strerror(errno));
    impl_->user_data_map[fd] = user_data;
}

void Poller::remove(int fd) {
    if (epoll_ctl(impl_->epfd, EPOLL_CTL_DEL, fd, nullptr) < 0)
        Log::warn("epoll_ctl DEL failed for fd {}: {}", fd, strerror(errno));
    impl_->user_data_map.erase(fd);
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
        void* ud = nullptr;
        auto it = impl_->user_data_map.find(ep.data.fd);
        if (it != impl_->user_data_map.end())
            ud = it->second;
        out[i] = Event{ep.data.fd, flags, ud, nullptr, 0, 0};
    }
    return n;
}

// -- buffer recycling (no-op for epoll) -------------------------------------

void Poller::recycle_buffer(uint16_t /*buffer_id*/) {
}

Poller::IoStats Poller::io_stats() const {
    return {};
}

// -- send (no-op for epoll — caller handles send inline) --------------------

bool Poller::submit_send(int /*fd*/, const void* /*data*/, size_t /*len*/, size_t* /*bytes_sent*/) {
    return true; // synchronous — caller must send inline
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
