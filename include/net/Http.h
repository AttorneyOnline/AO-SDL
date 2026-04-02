/**
 * @file Http.h
 * @brief httplib-compatible API surface (no implementation).
 *
 * This header reproduces the public API of cpp-httplib (yhirose/cpp-httplib)
 * so that existing code can compile against it. Implementations will be
 * provided per-platform using the platform/ socket abstraction layer.
 */
#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <ostream>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#if defined(_MSC_VER)
#ifdef _WIN64
using ssize_t = __int64;
#else
using ssize_t = long;
#endif
#endif // _MSC_VER
#else
#include <sys/socket.h>
#endif

// ---------------------------------------------------------------------------
// Configuration defaults (match cpp-httplib)
// ---------------------------------------------------------------------------

#ifndef CPPHTTPLIB_KEEPALIVE_TIMEOUT_SECOND
#define CPPHTTPLIB_KEEPALIVE_TIMEOUT_SECOND 5
#endif

#ifndef CPPHTTPLIB_KEEPALIVE_MAX_COUNT
#define CPPHTTPLIB_KEEPALIVE_MAX_COUNT 100
#endif

#ifndef CPPHTTPLIB_CONNECTION_TIMEOUT_SECOND
#define CPPHTTPLIB_CONNECTION_TIMEOUT_SECOND 300
#endif

#ifndef CPPHTTPLIB_CONNECTION_TIMEOUT_USECOND
#define CPPHTTPLIB_CONNECTION_TIMEOUT_USECOND 0
#endif

#ifndef CPPHTTPLIB_SERVER_READ_TIMEOUT_SECOND
#define CPPHTTPLIB_SERVER_READ_TIMEOUT_SECOND 5
#endif

#ifndef CPPHTTPLIB_SERVER_READ_TIMEOUT_USECOND
#define CPPHTTPLIB_SERVER_READ_TIMEOUT_USECOND 0
#endif

#ifndef CPPHTTPLIB_SERVER_WRITE_TIMEOUT_SECOND
#define CPPHTTPLIB_SERVER_WRITE_TIMEOUT_SECOND 5
#endif

#ifndef CPPHTTPLIB_SERVER_WRITE_TIMEOUT_USECOND
#define CPPHTTPLIB_SERVER_WRITE_TIMEOUT_USECOND 0
#endif

#ifndef CPPHTTPLIB_CLIENT_READ_TIMEOUT_SECOND
#define CPPHTTPLIB_CLIENT_READ_TIMEOUT_SECOND 300
#endif

#ifndef CPPHTTPLIB_CLIENT_READ_TIMEOUT_USECOND
#define CPPHTTPLIB_CLIENT_READ_TIMEOUT_USECOND 0
#endif

#ifndef CPPHTTPLIB_CLIENT_WRITE_TIMEOUT_SECOND
#define CPPHTTPLIB_CLIENT_WRITE_TIMEOUT_SECOND 5
#endif

#ifndef CPPHTTPLIB_CLIENT_WRITE_TIMEOUT_USECOND
#define CPPHTTPLIB_CLIENT_WRITE_TIMEOUT_USECOND 0
#endif

#ifndef CPPHTTPLIB_IDLE_INTERVAL_SECOND
#define CPPHTTPLIB_IDLE_INTERVAL_SECOND 0
#endif

#ifndef CPPHTTPLIB_IDLE_INTERVAL_USECOND
#define CPPHTTPLIB_IDLE_INTERVAL_USECOND 0
#endif

#ifndef CPPHTTPLIB_REDIRECT_MAX_COUNT
#define CPPHTTPLIB_REDIRECT_MAX_COUNT 20
#endif

#ifndef CPPHTTPLIB_PAYLOAD_MAX_LENGTH
#define CPPHTTPLIB_PAYLOAD_MAX_LENGTH ((std::numeric_limits<size_t>::max)())
#endif

#ifndef CPPHTTPLIB_TCP_NODELAY
#define CPPHTTPLIB_TCP_NODELAY false
#endif

#ifndef CPPHTTPLIB_IPV6_V6ONLY
#define CPPHTTPLIB_IPV6_V6ONLY false
#endif

#ifndef CPPHTTPLIB_THREAD_POOL_COUNT
#define CPPHTTPLIB_THREAD_POOL_COUNT                                                                                   \
    ((std::max)(8u, std::thread::hardware_concurrency() > 0 ? std::thread::hardware_concurrency() - 1 : 0))
#endif

#include "platform/Socket.h"

namespace http {

// Use platform socket types
using socket_t = int; // fd on POSIX, cast from SOCKET on Windows
constexpr socket_t INVALID_SOCKET_VALUE = -1;

// ===========================================================================
// detail namespace — minimal surface needed by public types
// ===========================================================================

// Forward declaration for use by detail::MatcherBase
struct Request;

namespace detail {

namespace case_ignore {

inline unsigned char to_lower(int c) {
    const static unsigned char table[256] = {
        0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,
        22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,
        44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  97,
        98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
        120, 121, 122, 91,  92,  93,  94,  95,  96,  97,  98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
        110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131,
        132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153,
        154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
        176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 224, 225, 226, 227, 228, 229,
        230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 215, 248, 249, 250, 251,
        252, 253, 254, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241,
        242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,
    };
    return table[(unsigned char)(char)c];
}

inline bool equal(const std::string& a, const std::string& b) {
    return a.size() == b.size() &&
           std::equal(a.begin(), a.end(), b.begin(), [](char ca, char cb) { return to_lower(ca) == to_lower(cb); });
}

struct equal_to {
    bool operator()(const std::string& a, const std::string& b) const {
        return equal(a, b);
    }
};

struct hash {
    size_t operator()(const std::string& key) const {
        return hash_core(key.data(), key.size(), 0);
    }

    size_t hash_core(const char* s, size_t l, size_t h) const {
        return (l == 0) ? h
                        : hash_core(s + 1, l - 1,
                                    (((std::numeric_limits<size_t>::max)() >> 6) & h * 33) ^
                                        static_cast<unsigned char>(to_lower(*s)));
    }
};

} // namespace case_ignore

class MatcherBase {
  public:
    virtual ~MatcherBase() = default;
    virtual bool match(Request& request) const = 0;
};

class PathParamsMatcher final : public MatcherBase {
  public:
    PathParamsMatcher(const std::string& pattern);
    bool match(Request& request) const override;

  private:
    static constexpr char separator = '/';
    std::vector<std::string> static_fragments_;
    std::vector<std::string> param_names_;
};

class RegexMatcher final : public MatcherBase {
  public:
    RegexMatcher(const std::string& pattern) : regex_(pattern) {
    }
    bool match(Request& request) const override;

  private:
    std::regex regex_;
};

} // namespace detail

// ===========================================================================
// Forward declarations
// ===========================================================================

struct Response;

// ===========================================================================
// Type aliases
// ===========================================================================

using Headers =
    std::unordered_multimap<std::string, std::string, detail::case_ignore::hash, detail::case_ignore::equal_to>;

// Stream — forward declared here, full definition below.
class Stream;

// write_headers is in detail:: but operates on http::Stream at namespace scope.
namespace detail {
ssize_t write_headers(::http::Stream& strm, Headers& headers);
} // namespace detail

using Params = std::multimap<std::string, std::string>;
using Match = std::smatch;

using Progress = std::function<bool(uint64_t current, uint64_t total)>;
using ResponseHandler = std::function<bool(const Response& response)>;

// ===========================================================================
// Enums
// ===========================================================================

enum StatusCode {
    // Information responses
    Continue_100 = 100,
    SwitchingProtocol_101 = 101,
    Processing_102 = 102,
    EarlyHints_103 = 103,

    // Successful responses
    OK_200 = 200,
    Created_201 = 201,
    Accepted_202 = 202,
    NonAuthoritativeInformation_203 = 203,
    NoContent_204 = 204,
    ResetContent_205 = 205,
    PartialContent_206 = 206,
    MultiStatus_207 = 207,
    AlreadyReported_208 = 208,
    IMUsed_226 = 226,

    // Redirection messages
    MultipleChoices_300 = 300,
    MovedPermanently_301 = 301,
    Found_302 = 302,
    SeeOther_303 = 303,
    NotModified_304 = 304,
    UseProxy_305 = 305,
    unused_306 = 306,
    TemporaryRedirect_307 = 307,
    PermanentRedirect_308 = 308,

    // Client error responses
    BadRequest_400 = 400,
    Unauthorized_401 = 401,
    PaymentRequired_402 = 402,
    Forbidden_403 = 403,
    NotFound_404 = 404,
    MethodNotAllowed_405 = 405,
    NotAcceptable_406 = 406,
    ProxyAuthenticationRequired_407 = 407,
    RequestTimeout_408 = 408,
    Conflict_409 = 409,
    Gone_410 = 410,
    LengthRequired_411 = 411,
    PreconditionFailed_412 = 412,
    PayloadTooLarge_413 = 413,
    UriTooLong_414 = 414,
    UnsupportedMediaType_415 = 415,
    RangeNotSatisfiable_416 = 416,
    ExpectationFailed_417 = 417,
    ImATeapot_418 = 418,
    MisdirectedRequest_421 = 421,
    UnprocessableContent_422 = 422,
    Locked_423 = 423,
    FailedDependency_424 = 424,
    TooEarly_425 = 425,
    UpgradeRequired_426 = 426,
    PreconditionRequired_428 = 428,
    TooManyRequests_429 = 429,
    RequestHeaderFieldsTooLarge_431 = 431,
    UnavailableForLegalReasons_451 = 451,

    // Server error responses
    InternalServerError_500 = 500,
    NotImplemented_501 = 501,
    BadGateway_502 = 502,
    ServiceUnavailable_503 = 503,
    GatewayTimeout_504 = 504,
    HttpVersionNotSupported_505 = 505,
    VariantAlsoNegotiates_506 = 506,
    InsufficientStorage_507 = 507,
    LoopDetected_508 = 508,
    NotExtended_510 = 510,
    NetworkAuthenticationRequired_511 = 511,
};

// ===========================================================================
// Multipart form data
// ===========================================================================

struct MultipartFormData {
    std::string name;
    std::string content;
    std::string filename;
    std::string content_type;
};
using MultipartFormDataItems = std::vector<MultipartFormData>;
using MultipartFormDataMap = std::multimap<std::string, MultipartFormData>;

// ===========================================================================
// DataSink
// ===========================================================================

class DataSink {
  public:
    DataSink() : os(&sb_), sb_(*this) {
    }

    DataSink(const DataSink&) = delete;
    DataSink& operator=(const DataSink&) = delete;
    DataSink(DataSink&&) = delete;
    DataSink& operator=(DataSink&&) = delete;

    std::function<bool(const char* data, size_t data_len)> write;
    std::function<bool()> is_writable;
    std::function<void()> done;
    std::function<void(const Headers& trailer)> done_with_trailer;
    std::ostream os;

  private:
    class data_sink_streambuf final : public std::streambuf {
      public:
        explicit data_sink_streambuf(DataSink& sink) : sink_(sink) {
        }

      protected:
        std::streamsize xsputn(const char* s, std::streamsize n) override {
            sink_.write(s, static_cast<size_t>(n));
            return n;
        }

      private:
        DataSink& sink_;
    };

    data_sink_streambuf sb_;
};

using ContentProvider = std::function<bool(size_t offset, size_t length, DataSink& sink)>;
using ContentProviderWithoutLength = std::function<bool(size_t offset, DataSink& sink)>;
using ContentProviderResourceReleaser = std::function<void(bool success)>;

struct MultipartFormDataProvider {
    std::string name;
    ContentProviderWithoutLength provider;
    std::string filename;
    std::string content_type;
};
using MultipartFormDataProviderItems = std::vector<MultipartFormDataProvider>;

using ContentReceiverWithProgress =
    std::function<bool(const char* data, size_t data_length, uint64_t offset, uint64_t total_length)>;

using ContentReceiver = std::function<bool(const char* data, size_t data_length)>;

using MultipartContentHeader = std::function<bool(const MultipartFormData& file)>;

// ===========================================================================
// ContentReader
// ===========================================================================

class ContentReader {
  public:
    using Reader = std::function<bool(ContentReceiver receiver)>;
    using MultipartReader = std::function<bool(MultipartContentHeader header, ContentReceiver receiver)>;

    ContentReader(Reader reader, MultipartReader multipart_reader)
        : reader_(std::move(reader)), multipart_reader_(std::move(multipart_reader)) {
    }

    bool operator()(MultipartContentHeader header, ContentReceiver receiver) const {
        return multipart_reader_(std::move(header), std::move(receiver));
    }

    bool operator()(ContentReceiver receiver) const {
        return reader_(std::move(receiver));
    }

    Reader reader_;
    MultipartReader multipart_reader_;
};

using Range = std::pair<ssize_t, ssize_t>;
using Ranges = std::vector<Range>;

// ===========================================================================
// Request
// ===========================================================================

struct Request {
    std::string method;
    std::string path;
    Params params;
    Headers headers;
    std::string body;

    std::string remote_addr;
    int remote_port = -1;
    std::string local_addr;
    int local_port = -1;

    // for server
    std::string version;
    std::string target;
    MultipartFormDataMap files;
    Ranges ranges;
    Match matches;
    std::unordered_map<std::string, std::string> path_params;

    // for client
    ResponseHandler response_handler;
    ContentReceiverWithProgress content_receiver;
    Progress progress;

    bool has_header(const std::string& key) const;
    std::string get_header_value(const std::string& key, const char* def = "", size_t id = 0) const;
    uint64_t get_header_value_u64(const std::string& key, uint64_t def = 0, size_t id = 0) const;
    size_t get_header_value_count(const std::string& key) const;
    void set_header(const std::string& key, const std::string& val);

    bool has_param(const std::string& key) const;
    std::string get_param_value(const std::string& key, size_t id = 0) const;
    size_t get_param_value_count(const std::string& key) const;

    bool is_multipart_form_data() const;

    bool has_file(const std::string& key) const;
    MultipartFormData get_file_value(const std::string& key) const;
    std::vector<MultipartFormData> get_file_values(const std::string& key) const;

    // private members
    size_t redirect_count_ = CPPHTTPLIB_REDIRECT_MAX_COUNT;
    size_t content_length_ = 0;
    ContentProvider content_provider_;
    bool is_chunked_content_provider_ = false;
    size_t authorization_count_ = 0;
};

// ===========================================================================
// Response
// ===========================================================================

struct Response {
    std::string version;
    int status = -1;
    std::string reason;
    Headers headers;
    std::string body;
    std::string location;

    bool has_header(const std::string& key) const;
    std::string get_header_value(const std::string& key, const char* def = "", size_t id = 0) const;
    uint64_t get_header_value_u64(const std::string& key, uint64_t def = 0, size_t id = 0) const;
    size_t get_header_value_count(const std::string& key) const;
    void set_header(const std::string& key, const std::string& val);

    void set_redirect(const std::string& url, int status = StatusCode::Found_302);
    void set_content(const char* s, size_t n, const std::string& content_type);
    void set_content(const std::string& s, const std::string& content_type);
    void set_content(std::string&& s, const std::string& content_type);

    void set_content_provider(size_t length, const std::string& content_type, ContentProvider provider,
                              ContentProviderResourceReleaser resource_releaser = nullptr);

    void set_content_provider(const std::string& content_type, ContentProviderWithoutLength provider,
                              ContentProviderResourceReleaser resource_releaser = nullptr);

    void set_chunked_content_provider(const std::string& content_type, ContentProviderWithoutLength provider,
                                      ContentProviderResourceReleaser resource_releaser = nullptr);

    void set_file_content(const std::string& path, const std::string& content_type);
    void set_file_content(const std::string& path);

    Response() = default;
    Response(const Response&) = default;
    Response& operator=(const Response&) = default;
    Response(Response&&) = default;
    Response& operator=(Response&&) = default;
    ~Response() {
        if (content_provider_resource_releaser_) {
            content_provider_resource_releaser_(content_provider_success_);
        }
    }

    // private members
    size_t content_length_ = 0;
    ContentProvider content_provider_;
    ContentProviderResourceReleaser content_provider_resource_releaser_;
    bool is_chunked_content_provider_ = false;
    bool content_provider_success_ = false;
    std::string file_content_path_;
    std::string file_content_content_type_;
};

// ===========================================================================
// Stream
// ===========================================================================

class Stream {
  public:
    virtual ~Stream() = default;

    virtual bool is_readable() const = 0;
    virtual bool is_writable() const = 0;

    virtual ssize_t read(char* ptr, size_t size) = 0;
    virtual ssize_t write(const char* ptr, size_t size) = 0;
    virtual void get_remote_ip_and_port(std::string& ip, int& port) const = 0;
    virtual void get_local_ip_and_port(std::string& ip, int& port) const = 0;
    virtual socket_t socket() const = 0;

    ssize_t write(const char* ptr);
    ssize_t write(const std::string& s);
};

// ===========================================================================
// TaskQueue / ThreadPool
// ===========================================================================

class TaskQueue {
  public:
    TaskQueue() = default;
    virtual ~TaskQueue() = default;

    virtual bool enqueue(std::function<void()> fn) = 0;
    virtual void shutdown() = 0;
    virtual void on_idle() {
    }
};

// ThreadPool was removed — the HTTP server uses std::jthread workers directly.

// ===========================================================================
// Logger / SocketOptions / free functions
// ===========================================================================

using Logger = std::function<void(const Request&, const Response&)>;
using SocketOptions = std::function<void(socket_t sock)>;

void default_socket_options(socket_t sock);
const char* status_message(int status);
std::string get_bearer_token_auth(const Request& req);

// ===========================================================================
// Error
// ===========================================================================

enum class Error {
    Success = 0,
    Unknown,
    Connection,
    BindIPAddress,
    Read,
    Write,
    ExceedRedirectCount,
    Canceled,
    SSLConnection,
    SSLLoadingCerts,
    SSLServerVerification,
    SSLServerHostnameVerification,
    UnsupportedMultipartBoundaryChars,
    Compression,
    ConnectionTimeout,
    ProxyConnection,
    SSLPeerCouldBeClosed_,
};

std::string to_string(Error error);
std::ostream& operator<<(std::ostream& os, const Error& obj);

// ===========================================================================
// Result
// ===========================================================================

class Result {
  public:
    Result() = default;
    Result(std::unique_ptr<Response>&& res, Error err, Headers&& request_headers = Headers{})
        : res_(std::move(res)), err_(err), request_headers_(std::move(request_headers)) {
    }

    operator bool() const {
        return res_ != nullptr;
    }
    bool operator==(std::nullptr_t) const {
        return res_ == nullptr;
    }
    bool operator!=(std::nullptr_t) const {
        return res_ != nullptr;
    }
    const Response& value() const {
        return *res_;
    }
    Response& value() {
        return *res_;
    }
    const Response& operator*() const {
        return *res_;
    }
    Response& operator*() {
        return *res_;
    }
    const Response* operator->() const {
        return res_.get();
    }
    Response* operator->() {
        return res_.get();
    }

    Error error() const {
        return err_;
    }

    bool has_request_header(const std::string& key) const;
    std::string get_request_header_value(const std::string& key, const char* def = "", size_t id = 0) const;
    uint64_t get_request_header_value_u64(const std::string& key, uint64_t def = 0, size_t id = 0) const;
    size_t get_request_header_value_count(const std::string& key) const;

  private:
    std::unique_ptr<Response> res_;
    Error err_ = Error::Unknown;
    Headers request_headers_;
};

// ===========================================================================
// Server
// ===========================================================================

class Server {
  public:
    using Handler = std::function<void(const Request&, Response&)>;
    using ExceptionHandler = std::function<void(const Request&, Response&, std::exception_ptr ep)>;

    enum class HandlerResponse { Handled, Unhandled };
    using HandlerWithResponse = std::function<HandlerResponse(const Request&, Response&)>;
    using HandlerWithContentReader =
        std::function<void(const Request&, Response&, const ContentReader& content_reader)>;
    using Expect100ContinueHandler = std::function<int(const Request&, Response&)>;

    Server();
    virtual ~Server();
    virtual bool is_valid() const;

    Server& Get(const std::string& pattern, Handler handler);
    Server& Post(const std::string& pattern, Handler handler);
    Server& Post(const std::string& pattern, HandlerWithContentReader handler);
    Server& Put(const std::string& pattern, Handler handler);
    Server& Put(const std::string& pattern, HandlerWithContentReader handler);
    Server& Patch(const std::string& pattern, Handler handler);
    Server& Patch(const std::string& pattern, HandlerWithContentReader handler);
    Server& Delete(const std::string& pattern, Handler handler);
    Server& Delete(const std::string& pattern, HandlerWithContentReader handler);
    Server& Options(const std::string& pattern, Handler handler);

    /// Result returned by an SSE handler to accept or reject a connection.
    struct SSEAcceptResult {
        bool accepted = false;     ///< true to open the SSE stream
        std::string session_token; ///< Optional token stored on the connection for TTL refresh
    };

    /// Register an SSE (Server-Sent Events) endpoint.
    /// The handler is called once when a client connects. It receives the
    /// Request (for auth/params) and a Response (for setting status/headers).
    /// Return an SSEAcceptResult — set accepted=true to open the stream.
    /// Accepted connections are held open and receive SSEEvents published
    /// to EventManager.
    using SSEHandler = std::function<SSEAcceptResult(const Request& req, Response& res)>;
    Server& SSE(const std::string& pattern, SSEHandler handler);

    /// Push an SSE event to all matching connections. Called by the poll loop;
    /// not intended for direct use — publish SSEEvent to EventManager instead.
    void push_sse(const std::string& event, const std::string& data, const std::string& area);

    /// Set a callback invoked during SSE keepalive to refresh session TTL.
    /// The callback receives the session token associated with each SSE connection.
    /// Called from the poll thread — the callback must handle its own locking.
    using SSESessionTouchFunc = std::function<void(const std::string& token)>;
    void set_sse_session_touch(SSESessionTouchFunc func);

    bool set_base_dir(const std::string& dir, const std::string& mount_point = std::string());
    bool set_mount_point(const std::string& mount_point, const std::string& dir, Headers headers = Headers());
    bool remove_mount_point(const std::string& mount_point);
    Server& set_file_extension_and_mimetype_mapping(const std::string& ext, const std::string& mime);
    Server& set_default_file_mimetype(const std::string& mime);
    Server& set_file_request_handler(Handler handler);

    template <class ErrorHandlerFunc>
    Server& set_error_handler(ErrorHandlerFunc&& handler) {
        return set_error_handler_core(std::forward<ErrorHandlerFunc>(handler),
                                      std::is_convertible<ErrorHandlerFunc, HandlerWithResponse>{});
    }

    Server& set_exception_handler(ExceptionHandler handler);
    Server& set_pre_routing_handler(HandlerWithResponse handler);
    Server& set_post_routing_handler(Handler handler);
    Server& set_expect_100_continue_handler(Expect100ContinueHandler handler);
    Server& set_logger(Logger logger);

    Server& set_address_family(int family);
    Server& set_tcp_nodelay(bool on);
    Server& set_ipv6_v6only(bool on);
    Server& set_socket_options(SocketOptions socket_options);

    Server& set_default_headers(Headers headers);
    Server& set_header_writer(std::function<ssize_t(Stream&, Headers&)> const& writer);

    Server& set_keep_alive_max_count(size_t count);
    Server& set_keep_alive_timeout(time_t sec);

    Server& set_read_timeout(time_t sec, time_t usec = 0);
    template <class Rep, class Period>
    Server& set_read_timeout(const std::chrono::duration<Rep, Period>& duration);

    Server& set_write_timeout(time_t sec, time_t usec = 0);
    template <class Rep, class Period>
    Server& set_write_timeout(const std::chrono::duration<Rep, Period>& duration);

    Server& set_idle_interval(time_t sec, time_t usec = 0);
    template <class Rep, class Period>
    Server& set_idle_interval(const std::chrono::duration<Rep, Period>& duration);

    Server& set_payload_max_length(size_t length);

    bool bind_to_port(const std::string& host, int port, int socket_flags = 0);
    int bind_to_any_port(const std::string& host, int socket_flags = 0);
    bool listen_after_bind();

    bool listen(const std::string& host, int port, int socket_flags = 0);

    bool is_running() const;
    void wait_until_ready() const;
    void stop();
    void decommission();

    std::function<TaskQueue*(void)> new_task_queue;

  protected:
    bool process_request(Stream& strm, const std::string& remote_addr, int remote_port, const std::string& local_addr,
                         int local_port, bool close_connection, bool& connection_closed,
                         const std::function<void(Request&)>& setup_request);

    std::atomic<socket_t> svr_sock_{INVALID_SOCKET_VALUE};
    size_t keep_alive_max_count_ = CPPHTTPLIB_KEEPALIVE_MAX_COUNT;
    time_t keep_alive_timeout_sec_ = CPPHTTPLIB_KEEPALIVE_TIMEOUT_SECOND;
    time_t read_timeout_sec_ = CPPHTTPLIB_SERVER_READ_TIMEOUT_SECOND;
    time_t read_timeout_usec_ = CPPHTTPLIB_SERVER_READ_TIMEOUT_USECOND;
    time_t write_timeout_sec_ = CPPHTTPLIB_SERVER_WRITE_TIMEOUT_SECOND;
    time_t write_timeout_usec_ = CPPHTTPLIB_SERVER_WRITE_TIMEOUT_USECOND;
    time_t idle_interval_sec_ = CPPHTTPLIB_IDLE_INTERVAL_SECOND;
    time_t idle_interval_usec_ = CPPHTTPLIB_IDLE_INTERVAL_USECOND;
    size_t payload_max_length_ = CPPHTTPLIB_PAYLOAD_MAX_LENGTH;

  private:
    static std::unique_ptr<detail::MatcherBase> make_matcher(const std::string& pattern);

    Server& set_error_handler_core(HandlerWithResponse handler, std::true_type);
    Server& set_error_handler_core(Handler handler, std::false_type);

    virtual bool process_and_close_socket(socket_t sock);

    std::atomic<bool> is_running_{false};

    Headers default_headers_;

  public:
    struct ServerState; // defined in HttpServer.cpp

  private:
    std::unique_ptr<ServerState> state_;
};

// ===========================================================================
// ClientImpl
// ===========================================================================

class ClientImpl {
  public:
    explicit ClientImpl(const std::string& host);
    explicit ClientImpl(const std::string& host, int port);
    explicit ClientImpl(const std::string& host, int port, const std::string& client_cert_path,
                        const std::string& client_key_path);
    virtual ~ClientImpl();
    virtual bool is_valid() const;

    // GET
    Result Get(const std::string& path);
    Result Get(const std::string& path, const Headers& headers);
    Result Get(const std::string& path, Progress progress);
    Result Get(const std::string& path, const Headers& headers, Progress progress);
    Result Get(const std::string& path, ContentReceiver content_receiver);
    Result Get(const std::string& path, const Headers& headers, ContentReceiver content_receiver);
    Result Get(const std::string& path, ContentReceiver content_receiver, Progress progress);
    Result Get(const std::string& path, const Headers& headers, ContentReceiver content_receiver, Progress progress);
    Result Get(const std::string& path, ResponseHandler response_handler, ContentReceiver content_receiver);
    Result Get(const std::string& path, const Headers& headers, ResponseHandler response_handler,
               ContentReceiver content_receiver);
    Result Get(const std::string& path, ResponseHandler response_handler, ContentReceiver content_receiver,
               Progress progress);
    Result Get(const std::string& path, const Headers& headers, ResponseHandler response_handler,
               ContentReceiver content_receiver, Progress progress);
    Result Get(const std::string& path, const Params& params, const Headers& headers, Progress progress = nullptr);
    Result Get(const std::string& path, const Params& params, const Headers& headers, ContentReceiver content_receiver,
               Progress progress = nullptr);
    Result Get(const std::string& path, const Params& params, const Headers& headers, ResponseHandler response_handler,
               ContentReceiver content_receiver, Progress progress = nullptr);

    // HEAD
    Result Head(const std::string& path);
    Result Head(const std::string& path, const Headers& headers);

    // POST
    Result Post(const std::string& path);
    Result Post(const std::string& path, const Headers& headers);
    Result Post(const std::string& path, const char* body, size_t content_length, const std::string& content_type);
    Result Post(const std::string& path, const Headers& headers, const char* body, size_t content_length,
                const std::string& content_type);
    Result Post(const std::string& path, const Headers& headers, const char* body, size_t content_length,
                const std::string& content_type, Progress progress);
    Result Post(const std::string& path, const std::string& body, const std::string& content_type);
    Result Post(const std::string& path, const std::string& body, const std::string& content_type, Progress progress);
    Result Post(const std::string& path, const Headers& headers, const std::string& body,
                const std::string& content_type);
    Result Post(const std::string& path, const Headers& headers, const std::string& body,
                const std::string& content_type, Progress progress);
    Result Post(const std::string& path, size_t content_length, ContentProvider content_provider,
                const std::string& content_type);
    Result Post(const std::string& path, ContentProviderWithoutLength content_provider,
                const std::string& content_type);
    Result Post(const std::string& path, const Headers& headers, size_t content_length,
                ContentProvider content_provider, const std::string& content_type);
    Result Post(const std::string& path, const Headers& headers, ContentProviderWithoutLength content_provider,
                const std::string& content_type);
    Result Post(const std::string& path, const Params& params);
    Result Post(const std::string& path, const Headers& headers, const Params& params);
    Result Post(const std::string& path, const Headers& headers, const Params& params, Progress progress);
    Result Post(const std::string& path, const MultipartFormDataItems& items);
    Result Post(const std::string& path, const Headers& headers, const MultipartFormDataItems& items);
    Result Post(const std::string& path, const Headers& headers, const MultipartFormDataItems& items,
                const std::string& boundary);
    Result Post(const std::string& path, const Headers& headers, const MultipartFormDataItems& items,
                const MultipartFormDataProviderItems& provider_items);

    // PUT
    Result Put(const std::string& path);
    Result Put(const std::string& path, const char* body, size_t content_length, const std::string& content_type);
    Result Put(const std::string& path, const Headers& headers, const char* body, size_t content_length,
               const std::string& content_type);
    Result Put(const std::string& path, const Headers& headers, const char* body, size_t content_length,
               const std::string& content_type, Progress progress);
    Result Put(const std::string& path, const std::string& body, const std::string& content_type);
    Result Put(const std::string& path, const std::string& body, const std::string& content_type, Progress progress);
    Result Put(const std::string& path, const Headers& headers, const std::string& body,
               const std::string& content_type);
    Result Put(const std::string& path, const Headers& headers, const std::string& body,
               const std::string& content_type, Progress progress);
    Result Put(const std::string& path, size_t content_length, ContentProvider content_provider,
               const std::string& content_type);
    Result Put(const std::string& path, ContentProviderWithoutLength content_provider, const std::string& content_type);
    Result Put(const std::string& path, const Headers& headers, size_t content_length, ContentProvider content_provider,
               const std::string& content_type);
    Result Put(const std::string& path, const Headers& headers, ContentProviderWithoutLength content_provider,
               const std::string& content_type);
    Result Put(const std::string& path, const Params& params);
    Result Put(const std::string& path, const Headers& headers, const Params& params);
    Result Put(const std::string& path, const Headers& headers, const Params& params, Progress progress);
    Result Put(const std::string& path, const MultipartFormDataItems& items);
    Result Put(const std::string& path, const Headers& headers, const MultipartFormDataItems& items);
    Result Put(const std::string& path, const Headers& headers, const MultipartFormDataItems& items,
               const std::string& boundary);
    Result Put(const std::string& path, const Headers& headers, const MultipartFormDataItems& items,
               const MultipartFormDataProviderItems& provider_items);

    // PATCH
    Result Patch(const std::string& path);
    Result Patch(const std::string& path, const char* body, size_t content_length, const std::string& content_type);
    Result Patch(const std::string& path, const char* body, size_t content_length, const std::string& content_type,
                 Progress progress);
    Result Patch(const std::string& path, const Headers& headers, const char* body, size_t content_length,
                 const std::string& content_type);
    Result Patch(const std::string& path, const Headers& headers, const char* body, size_t content_length,
                 const std::string& content_type, Progress progress);
    Result Patch(const std::string& path, const std::string& body, const std::string& content_type);
    Result Patch(const std::string& path, const std::string& body, const std::string& content_type, Progress progress);
    Result Patch(const std::string& path, const Headers& headers, const std::string& body,
                 const std::string& content_type);
    Result Patch(const std::string& path, const Headers& headers, const std::string& body,
                 const std::string& content_type, Progress progress);
    Result Patch(const std::string& path, size_t content_length, ContentProvider content_provider,
                 const std::string& content_type);
    Result Patch(const std::string& path, ContentProviderWithoutLength content_provider,
                 const std::string& content_type);
    Result Patch(const std::string& path, const Headers& headers, size_t content_length,
                 ContentProvider content_provider, const std::string& content_type);
    Result Patch(const std::string& path, const Headers& headers, ContentProviderWithoutLength content_provider,
                 const std::string& content_type);

    // DELETE
    Result Delete(const std::string& path);
    Result Delete(const std::string& path, const Headers& headers);
    Result Delete(const std::string& path, const char* body, size_t content_length, const std::string& content_type);
    Result Delete(const std::string& path, const char* body, size_t content_length, const std::string& content_type,
                  Progress progress);
    Result Delete(const std::string& path, const Headers& headers, const char* body, size_t content_length,
                  const std::string& content_type);
    Result Delete(const std::string& path, const Headers& headers, const char* body, size_t content_length,
                  const std::string& content_type, Progress progress);
    Result Delete(const std::string& path, const std::string& body, const std::string& content_type);
    Result Delete(const std::string& path, const std::string& body, const std::string& content_type, Progress progress);
    Result Delete(const std::string& path, const Headers& headers, const std::string& body,
                  const std::string& content_type);
    Result Delete(const std::string& path, const Headers& headers, const std::string& body,
                  const std::string& content_type, Progress progress);

    // OPTIONS
    Result Options(const std::string& path);
    Result Options(const std::string& path, const Headers& headers);

    bool send(Request& req, Response& res, Error& error);
    Result send(const Request& req);

    void stop();

    std::string host() const;
    int port() const;

    size_t is_socket_open() const;
    socket_t socket() const;

    void set_hostname_addr_map(std::map<std::string, std::string> addr_map);
    void set_default_headers(Headers headers);
    void set_header_writer(std::function<ssize_t(Stream&, Headers&)> const& writer);

    void set_address_family(int family);
    void set_tcp_nodelay(bool on);
    void set_ipv6_v6only(bool on);
    void set_socket_options(SocketOptions socket_options);

    void set_connection_timeout(time_t sec, time_t usec = 0);
    template <class Rep, class Period>
    void set_connection_timeout(const std::chrono::duration<Rep, Period>& duration);

    void set_read_timeout(time_t sec, time_t usec = 0);
    template <class Rep, class Period>
    void set_read_timeout(const std::chrono::duration<Rep, Period>& duration);

    void set_write_timeout(time_t sec, time_t usec = 0);
    template <class Rep, class Period>
    void set_write_timeout(const std::chrono::duration<Rep, Period>& duration);

    void set_basic_auth(const std::string& username, const std::string& password);
    void set_bearer_token_auth(const std::string& token);

    void set_keep_alive(bool on);
    void set_follow_location(bool on);
    void set_url_encode(bool on);
    void set_compress(bool on);
    void set_decompress(bool on);
    void set_interface(const std::string& intf);

    void set_proxy(const std::string& host, int port);
    void set_proxy_basic_auth(const std::string& username, const std::string& password);
    void set_proxy_bearer_token_auth(const std::string& token);

    void set_logger(Logger logger);

  protected:
    std::string host_;
    int port_;
    std::string host_and_port_;

    std::optional<platform::Socket> socket_;
    mutable std::mutex socket_mutex_;

    Headers default_headers_;

    std::string client_cert_path_;
    std::string client_key_path_;

    time_t connection_timeout_sec_ = CPPHTTPLIB_CONNECTION_TIMEOUT_SECOND;
    time_t connection_timeout_usec_ = CPPHTTPLIB_CONNECTION_TIMEOUT_USECOND;
    time_t read_timeout_sec_ = CPPHTTPLIB_CLIENT_READ_TIMEOUT_SECOND;
    time_t read_timeout_usec_ = CPPHTTPLIB_CLIENT_READ_TIMEOUT_USECOND;
    time_t write_timeout_sec_ = CPPHTTPLIB_CLIENT_WRITE_TIMEOUT_SECOND;
    time_t write_timeout_usec_ = CPPHTTPLIB_CLIENT_WRITE_TIMEOUT_USECOND;

    std::string basic_auth_username_;
    std::string basic_auth_password_;
    std::string bearer_token_auth_token_;

    bool keep_alive_ = false;
    bool follow_location_ = false;
    bool url_encode_ = true;

    int address_family_ = AF_UNSPEC;
    bool tcp_nodelay_ = CPPHTTPLIB_TCP_NODELAY;
    bool ipv6_v6only_ = CPPHTTPLIB_IPV6_V6ONLY;
    SocketOptions socket_options_ = nullptr;

    bool ssl_ = false;
    bool compress_ = false;
    bool decompress_ = true;

    std::string interface_;

    std::string proxy_host_;
    int proxy_port_ = -1;
    std::string proxy_basic_auth_username_;
    std::string proxy_basic_auth_password_;
    std::string proxy_bearer_token_auth_token_;

    Logger logger_;

    std::atomic<bool> is_stopping_{false};

  private:
    Result execute_request(const std::string& method, const std::string& path, const Headers& headers,
                           const std::string& body, ContentReceiver content_receiver = nullptr);
    bool ensure_connected(bool& is_fresh);

    int connection_timeout_ms() const;
    int read_timeout_ms() const;
    int write_timeout_ms() const;

    bool is_ssl() const;
};

// ===========================================================================
// Client (universal wrapper)
// ===========================================================================

class Client {
  public:
    explicit Client(const std::string& scheme_host_port);
    explicit Client(const std::string& scheme_host_port, const std::string& client_cert_path,
                    const std::string& client_key_path);
    explicit Client(const std::string& host, int port);
    explicit Client(const std::string& host, int port, const std::string& client_cert_path,
                    const std::string& client_key_path);

    Client(Client&&) = default;
    Client& operator=(Client&&) = default;
    ~Client();

    bool is_valid() const;

    // All HTTP methods — delegated to ClientImpl
    Result Get(const std::string& path);
    Result Get(const std::string& path, const Headers& headers);
    Result Get(const std::string& path, Progress progress);
    Result Get(const std::string& path, const Headers& headers, Progress progress);
    Result Get(const std::string& path, ContentReceiver content_receiver);
    Result Get(const std::string& path, const Headers& headers, ContentReceiver content_receiver);
    Result Get(const std::string& path, ContentReceiver content_receiver, Progress progress);
    Result Get(const std::string& path, const Headers& headers, ContentReceiver content_receiver, Progress progress);
    Result Get(const std::string& path, ResponseHandler response_handler, ContentReceiver content_receiver);
    Result Get(const std::string& path, const Headers& headers, ResponseHandler response_handler,
               ContentReceiver content_receiver);
    Result Get(const std::string& path, const Headers& headers, ResponseHandler response_handler,
               ContentReceiver content_receiver, Progress progress);
    Result Get(const std::string& path, ResponseHandler response_handler, ContentReceiver content_receiver,
               Progress progress);
    Result Get(const std::string& path, const Params& params, const Headers& headers, Progress progress = nullptr);
    Result Get(const std::string& path, const Params& params, const Headers& headers, ContentReceiver content_receiver,
               Progress progress = nullptr);
    Result Get(const std::string& path, const Params& params, const Headers& headers, ResponseHandler response_handler,
               ContentReceiver content_receiver, Progress progress = nullptr);

    Result Head(const std::string& path);
    Result Head(const std::string& path, const Headers& headers);

    Result Post(const std::string& path);
    Result Post(const std::string& path, const Headers& headers);
    Result Post(const std::string& path, const char* body, size_t content_length, const std::string& content_type);
    Result Post(const std::string& path, const Headers& headers, const char* body, size_t content_length,
                const std::string& content_type);
    Result Post(const std::string& path, const Headers& headers, const char* body, size_t content_length,
                const std::string& content_type, Progress progress);
    Result Post(const std::string& path, const std::string& body, const std::string& content_type);
    Result Post(const std::string& path, const std::string& body, const std::string& content_type, Progress progress);
    Result Post(const std::string& path, const Headers& headers, const std::string& body,
                const std::string& content_type);
    Result Post(const std::string& path, const Headers& headers, const std::string& body,
                const std::string& content_type, Progress progress);
    Result Post(const std::string& path, size_t content_length, ContentProvider content_provider,
                const std::string& content_type);
    Result Post(const std::string& path, ContentProviderWithoutLength content_provider,
                const std::string& content_type);
    Result Post(const std::string& path, const Headers& headers, size_t content_length,
                ContentProvider content_provider, const std::string& content_type);
    Result Post(const std::string& path, const Headers& headers, ContentProviderWithoutLength content_provider,
                const std::string& content_type);
    Result Post(const std::string& path, const Params& params);
    Result Post(const std::string& path, const Headers& headers, const Params& params);
    Result Post(const std::string& path, const Headers& headers, const Params& params, Progress progress);
    Result Post(const std::string& path, const MultipartFormDataItems& items);
    Result Post(const std::string& path, const Headers& headers, const MultipartFormDataItems& items);
    Result Post(const std::string& path, const Headers& headers, const MultipartFormDataItems& items,
                const std::string& boundary);
    Result Post(const std::string& path, const Headers& headers, const MultipartFormDataItems& items,
                const MultipartFormDataProviderItems& provider_items);

    Result Put(const std::string& path);
    Result Put(const std::string& path, const char* body, size_t content_length, const std::string& content_type);
    Result Put(const std::string& path, const Headers& headers, const char* body, size_t content_length,
               const std::string& content_type);
    Result Put(const std::string& path, const Headers& headers, const char* body, size_t content_length,
               const std::string& content_type, Progress progress);
    Result Put(const std::string& path, const std::string& body, const std::string& content_type);
    Result Put(const std::string& path, const std::string& body, const std::string& content_type, Progress progress);
    Result Put(const std::string& path, const Headers& headers, const std::string& body,
               const std::string& content_type);
    Result Put(const std::string& path, const Headers& headers, const std::string& body,
               const std::string& content_type, Progress progress);
    Result Put(const std::string& path, size_t content_length, ContentProvider content_provider,
               const std::string& content_type);
    Result Put(const std::string& path, ContentProviderWithoutLength content_provider, const std::string& content_type);
    Result Put(const std::string& path, const Headers& headers, size_t content_length, ContentProvider content_provider,
               const std::string& content_type);
    Result Put(const std::string& path, const Headers& headers, ContentProviderWithoutLength content_provider,
               const std::string& content_type);
    Result Put(const std::string& path, const Params& params);
    Result Put(const std::string& path, const Headers& headers, const Params& params);
    Result Put(const std::string& path, const Headers& headers, const Params& params, Progress progress);
    Result Put(const std::string& path, const MultipartFormDataItems& items);
    Result Put(const std::string& path, const Headers& headers, const MultipartFormDataItems& items);
    Result Put(const std::string& path, const Headers& headers, const MultipartFormDataItems& items,
               const std::string& boundary);
    Result Put(const std::string& path, const Headers& headers, const MultipartFormDataItems& items,
               const MultipartFormDataProviderItems& provider_items);

    Result Patch(const std::string& path);
    Result Patch(const std::string& path, const char* body, size_t content_length, const std::string& content_type);
    Result Patch(const std::string& path, const char* body, size_t content_length, const std::string& content_type,
                 Progress progress);
    Result Patch(const std::string& path, const Headers& headers, const char* body, size_t content_length,
                 const std::string& content_type);
    Result Patch(const std::string& path, const Headers& headers, const char* body, size_t content_length,
                 const std::string& content_type, Progress progress);
    Result Patch(const std::string& path, const std::string& body, const std::string& content_type);
    Result Patch(const std::string& path, const std::string& body, const std::string& content_type, Progress progress);
    Result Patch(const std::string& path, const Headers& headers, const std::string& body,
                 const std::string& content_type);
    Result Patch(const std::string& path, const Headers& headers, const std::string& body,
                 const std::string& content_type, Progress progress);
    Result Patch(const std::string& path, size_t content_length, ContentProvider content_provider,
                 const std::string& content_type);
    Result Patch(const std::string& path, ContentProviderWithoutLength content_provider,
                 const std::string& content_type);
    Result Patch(const std::string& path, const Headers& headers, size_t content_length,
                 ContentProvider content_provider, const std::string& content_type);
    Result Patch(const std::string& path, const Headers& headers, ContentProviderWithoutLength content_provider,
                 const std::string& content_type);

    Result Delete(const std::string& path);
    Result Delete(const std::string& path, const Headers& headers);
    Result Delete(const std::string& path, const char* body, size_t content_length, const std::string& content_type);
    Result Delete(const std::string& path, const char* body, size_t content_length, const std::string& content_type,
                  Progress progress);
    Result Delete(const std::string& path, const Headers& headers, const char* body, size_t content_length,
                  const std::string& content_type);
    Result Delete(const std::string& path, const Headers& headers, const char* body, size_t content_length,
                  const std::string& content_type, Progress progress);
    Result Delete(const std::string& path, const std::string& body, const std::string& content_type);
    Result Delete(const std::string& path, const std::string& body, const std::string& content_type, Progress progress);
    Result Delete(const std::string& path, const Headers& headers, const std::string& body,
                  const std::string& content_type);
    Result Delete(const std::string& path, const Headers& headers, const std::string& body,
                  const std::string& content_type, Progress progress);

    Result Options(const std::string& path);
    Result Options(const std::string& path, const Headers& headers);

    bool send(Request& req, Response& res, Error& error);
    Result send(const Request& req);

    void stop();

    std::string host() const;
    int port() const;

    size_t is_socket_open() const;
    socket_t socket() const;

    void set_hostname_addr_map(std::map<std::string, std::string> addr_map);
    void set_default_headers(Headers headers);
    void set_header_writer(std::function<ssize_t(Stream&, Headers&)> const& writer);

    void set_address_family(int family);
    void set_tcp_nodelay(bool on);
    void set_socket_options(SocketOptions socket_options);

    void set_connection_timeout(time_t sec, time_t usec = 0);
    template <class Rep, class Period>
    void set_connection_timeout(const std::chrono::duration<Rep, Period>& duration);

    void set_read_timeout(time_t sec, time_t usec = 0);
    template <class Rep, class Period>
    void set_read_timeout(const std::chrono::duration<Rep, Period>& duration);

    void set_write_timeout(time_t sec, time_t usec = 0);
    template <class Rep, class Period>
    void set_write_timeout(const std::chrono::duration<Rep, Period>& duration);

    void set_basic_auth(const std::string& username, const std::string& password);
    void set_bearer_token_auth(const std::string& token);

    void set_keep_alive(bool on);
    void set_follow_location(bool on);
    void set_url_encode(bool on);
    void set_compress(bool on);
    void set_decompress(bool on);
    void set_interface(const std::string& intf);

    void set_proxy(const std::string& host, int port);
    void set_proxy_basic_auth(const std::string& username, const std::string& password);
    void set_proxy_bearer_token_auth(const std::string& token);

    void set_logger(Logger logger);

  private:
    std::unique_ptr<ClientImpl> cli_;
};

} // namespace http
