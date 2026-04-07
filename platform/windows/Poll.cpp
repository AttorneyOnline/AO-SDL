#include "platform/Poll.h"
#include "platform/Socket.h"

#ifndef _WIN32
#error "This file should only be compiled on Windows"
#endif

#include <winsock2.h>

#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace platform {

struct Poller::Impl {
    struct Entry {
        uint32_t interest;
        void* user_data;
    };

    std::vector<WSAPOLLFD> pollfds;
    std::unordered_map<int, Entry> entries;
    SOCKET notify_read = INVALID_SOCKET;
    SOCKET notify_write = INVALID_SOCKET;
};

Poller::Poller(unsigned /*io_buffers*/) : impl_(std::make_unique<Impl>()) {
}
Poller::~Poller() = default;
Poller::Poller(Poller&&) noexcept = default;
Poller& Poller::operator=(Poller&&) noexcept = default;

static SHORT to_wsa_events(uint32_t interest) {
    SHORT ev = 0;
    if (interest & Poller::Readable)
        ev |= POLLIN;
    if (interest & Poller::Writable)
        ev |= POLLOUT;
    return ev;
}

// -- fd-based (core) --------------------------------------------------------

void Poller::add(int fd, uint32_t interest, void* user_data) {
    WSAPOLLFD pfd{};
    pfd.fd = static_cast<SOCKET>(fd);
    pfd.events = to_wsa_events(interest);
    impl_->pollfds.push_back(pfd);
    impl_->entries[fd] = {interest, user_data};
}

void Poller::modify(int fd, uint32_t interest, void* user_data) {
    for (auto& pfd : impl_->pollfds) {
        if (static_cast<int>(pfd.fd) == fd) {
            pfd.events = to_wsa_events(interest);
            break;
        }
    }
    impl_->entries[fd] = {interest, user_data};
}

void Poller::remove(int fd) {
    auto& pfds = impl_->pollfds;
    for (auto it = pfds.begin(); it != pfds.end(); ++it) {
        if (static_cast<int>(it->fd) == fd) {
            pfds.erase(it);
            break;
        }
    }
    impl_->entries.erase(fd);
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
    if (impl_->pollfds.empty()) {
        if (timeout_ms > 0)
            Sleep(static_cast<DWORD>(timeout_ms));
        return 0;
    }

    int n = WSAPoll(impl_->pollfds.data(), static_cast<ULONG>(impl_->pollfds.size()), timeout_ms);
    if (n <= 0)
        return 0;

    int count = 0;
    for (auto& pfd : impl_->pollfds) {
        if (count >= max_events)
            break;
        if (pfd.revents == 0)
            continue;

        uint32_t flags = 0;
        if (pfd.revents & POLLIN)
            flags |= Readable;
        if (pfd.revents & POLLOUT)
            flags |= Writable;
        if (pfd.revents & POLLERR)
            flags |= Error;
        if (pfd.revents & POLLHUP)
            flags |= HangUp;

        int fd = static_cast<int>(pfd.fd);
        void* ud = nullptr;
        auto it = impl_->entries.find(fd);
        if (it != impl_->entries.end())
            ud = it->second.user_data;

        out[count++] = Event{fd, flags, ud, nullptr, 0, 0};
    }
    return count;
}

// -- buffer recycling (no-op for WSAPoll) -----------------------------------

void Poller::recycle_buffer(uint16_t /*buffer_id*/) {
}

Poller::IoStats Poller::io_stats() const {
    return {};
}

// -- send (no-op for WSAPoll — caller handles send inline) ------------------

bool Poller::submit_send(int /*fd*/, const void* /*data*/, size_t /*len*/, size_t* /*bytes_sent*/) {
    return true; // synchronous — caller must send inline
}

// -- notifier (self-connected loopback TCP pair) ----------------------------

int Poller::create_notifier() {
    // Windows lacks eventfd/pipe for sockets. Use a self-connected TCP pair.
    SOCKET listener = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET)
        throw std::runtime_error("Poller::create_notifier: socket() failed");

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    if (::bind(listener, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR || ::listen(listener, 1) == SOCKET_ERROR) {
        closesocket(listener);
        throw std::runtime_error("Poller::create_notifier: bind/listen failed");
    }

    int len = sizeof(addr);
    getsockname(listener, (sockaddr*)&addr, &len);

    impl_->notify_write = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (impl_->notify_write == INVALID_SOCKET) {
        closesocket(listener);
        throw std::runtime_error("Poller::create_notifier: write socket() failed");
    }

    if (::connect(impl_->notify_write, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(impl_->notify_write);
        impl_->notify_write = INVALID_SOCKET;
        closesocket(listener);
        throw std::runtime_error("Poller::create_notifier: connect() failed");
    }

    impl_->notify_read = ::accept(listener, nullptr, nullptr);
    closesocket(listener);
    if (impl_->notify_read == INVALID_SOCKET) {
        closesocket(impl_->notify_write);
        impl_->notify_write = INVALID_SOCKET;
        throw std::runtime_error("Poller::create_notifier: accept() failed");
    }

    // Make read end non-blocking
    u_long mode = 1;
    ioctlsocket(impl_->notify_read, FIONBIO, &mode);

    add(static_cast<int>(impl_->notify_read), Readable);
    return static_cast<int>(impl_->notify_read);
}

void Poller::notify() {
    if (impl_->notify_write != INVALID_SOCKET) {
        char c = 1;
        ::send(impl_->notify_write, &c, 1, 0);
    }
}

void Poller::drain_notifier() {
    if (impl_->notify_read != INVALID_SOCKET) {
        char buf[64];
        while (::recv(impl_->notify_read, buf, sizeof(buf), 0) > 0) {
        }
    }
}

} // namespace platform
