#pragma once

#include <cstdint>
#include <memory>

namespace platform {

class Socket;

/// Scalable I/O event notification.
/// Wraps epoll (Linux), kqueue (macOS), or WSAPoll/IOCP (Windows).
///
/// Implemented per-platform in platform/{macos,linux,windows}.
class Poller {
  public:
    Poller();
    ~Poller();
    Poller(Poller&& other) noexcept;
    Poller& operator=(Poller&& other) noexcept;
    Poller(const Poller&) = delete;
    Poller& operator=(const Poller&) = delete;

    /// Interest / readiness flags (bitmask).
    enum EventFlags : uint32_t {
        Readable = 1 << 0,
        Writable = 1 << 1,
        Error = 1 << 2,
        HangUp = 1 << 3,
    };

    /// A readiness event returned by poll().
    struct Event {
        int fd;          ///< File descriptor (matches Socket::fd()).
        uint32_t flags;  ///< Bitmask of EventFlags indicating what is ready.
        void* user_data; ///< Opaque pointer passed during add().
    };

    /// Register a socket for event notification.
    /// @param sock      The socket to monitor.
    /// @param interest  Bitmask of EventFlags to watch for.
    /// @param user_data Opaque pointer returned in Event on readiness.
    void add(const Socket& sock, uint32_t interest, void* user_data = nullptr);
    void add(int fd, uint32_t interest, void* user_data = nullptr);

    /// Change the interest set for an already-registered socket.
    void modify(const Socket& sock, uint32_t interest, void* user_data = nullptr);
    void modify(int fd, uint32_t interest, void* user_data = nullptr);

    /// Remove a socket from the poll set.
    void remove(const Socket& sock);
    void remove(int fd);

    /// Block until at least one event is ready, or timeout expires.
    /// @param out        Output array for ready events.
    /// @param max_events Capacity of the out array.
    /// @param timeout_ms -1 = block indefinitely, 0 = non-blocking.
    /// @return Number of events written to out.
    int poll(Event* out, int max_events, int timeout_ms = -1);

    /// Create a notification channel that can wake poll() from another thread.
    /// The returned fd is automatically added to the poll set with Readable interest.
    /// On Linux this uses eventfd, on macOS a kqueue user event, on Windows a self-pipe.
    /// @return The notifier fd (for identification in events).
    int create_notifier();

    /// Signal the notifier to wake the poll thread. Thread-safe.
    void notify();

    /// Drain any pending notification bytes. Call after poll() returns
    /// the notifier fd as readable, to reset the notifier for next use.
    void drain_notifier();

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace platform
