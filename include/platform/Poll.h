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
    /// @param io_buffers  Number of provided I/O buffers for completion-mode
    ///                    backends (io_uring).  Each buffer is 8 KB.  Ignored
    ///                    by readiness backends (epoll/kqueue/WSAPoll).
    ///                    Must be a power of two.  Default: 4096 (32 MB).
    explicit Poller(unsigned io_buffers = 4096);
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
        SendDone = 1 << 4, ///< Async send completed (data_len = bytes sent).
    };

    /// An event returned by poll().
    ///
    /// In readiness mode (epoll/kqueue), only fd + flags are set and the
    /// caller must issue its own recv()/send().
    ///
    /// In completion mode (io_uring), completed I/O results may carry data
    /// directly: if data_len > 0, the buffer pointed to by data contains
    /// the bytes already read by the kernel.  The caller must call
    /// recycle_buffer() when done with the data so the buffer can be reused.
    struct Event {
        int fd;              ///< File descriptor (matches Socket::fd()).
        uint32_t flags;      ///< Bitmask of EventFlags indicating what is ready.
        void* user_data;     ///< Opaque pointer passed during add().
        const void* data;    ///< Completed read buffer (nullptr for readiness backends).
        size_t data_len;     ///< Bytes in data (0 for readiness backends).
        uint16_t buffer_id;  ///< Opaque buffer ID for recycle_buffer() (io_uring only).
    };

    /// Return a completed-read buffer to the pool.  No-op on readiness backends.
    /// Must be called exactly once for each Event where data_len > 0.
    void recycle_buffer(uint16_t buffer_id);

    /// io_uring diagnostic counters (all zero on readiness backends).
    struct IoStats {
        uint64_t recv_submitted = 0;
        uint64_t recv_completed = 0;
        uint64_t recv_enobufs = 0;
        uint64_t send_submitted = 0;
        uint64_t send_completed = 0;
        uint64_t send_partial = 0;
        uint64_t send_errors = 0;
        uint64_t sqe_full = 0;
        uint64_t cqe_reaped = 0;
    };
    IoStats io_stats() const;

    /// Submit an asynchronous send.  The caller must keep the data buffer
    /// alive until a SendDone event is delivered for this fd.
    ///
    /// On completion backends (io_uring), the send is fully asynchronous:
    /// returns false, and poll() will later deliver a SendDone event with
    /// data_len set to total bytes sent.  Partial writes are handled
    /// internally by resubmitting.
    ///
    /// On readiness backends (epoll/kqueue), the send is performed inline
    /// as a blocking write: returns true and data_len in the result is the
    /// total bytes sent.  No SendDone event is delivered.
    ///
    /// @return true if the send completed synchronously.
    bool submit_send(int fd, const void* data, size_t len, size_t* bytes_sent = nullptr);

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
