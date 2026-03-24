/**
 * @file apple_httplib.mm
 * @brief NSURLSession-based httplib::Client implementation for Apple platforms.
 */
#import "apple_httplib.h"

#import <Foundation/Foundation.h>

#include "utils/Log.h"

#include <condition_variable>
#include <mutex>

namespace httplib {

struct Client::Impl {
    std::string host;
    NSURLSession *session = nil;
    int connect_timeout = 5;
    int read_timeout = 10;

    Impl(const std::string &h) : host(h) {
        @autoreleasepool {
            NSURLSessionConfiguration *config = [NSURLSessionConfiguration ephemeralSessionConfiguration];
            config.timeoutIntervalForResource = read_timeout;
            config.timeoutIntervalForRequest = connect_timeout;
            config.HTTPShouldSetCookies = NO;
            session = [NSURLSession sessionWithConfiguration:config];
        }
    }

    ~Impl() {
        @autoreleasepool {
            [session invalidateAndCancel];
            session = nil;
        }
    }

    void update_timeouts() {
        @autoreleasepool {
            [session invalidateAndCancel];
            NSURLSessionConfiguration *config = [NSURLSessionConfiguration ephemeralSessionConfiguration];
            config.timeoutIntervalForResource = read_timeout;
            config.timeoutIntervalForRequest = connect_timeout;
            config.HTTPShouldSetCookies = NO;
            session = [NSURLSession sessionWithConfiguration:config];
        }
    }

    NSURL *make_url(const std::string &path) {
        std::string full = host + path;
        NSString *str = [NSString stringWithUTF8String:full.c_str()];
        return [NSURL URLWithString:str];
    }
};

Client::Client(const std::string &host) : impl_(std::make_unique<Impl>(host)) {}
Client::~Client() = default;

void Client::set_connection_timeout(int seconds) {
    impl_->connect_timeout = seconds;
    impl_->update_timeouts();
}

void Client::set_read_timeout(int seconds) {
    impl_->read_timeout = seconds;
    impl_->update_timeouts();
}

void Client::set_keep_alive(bool /*enabled*/) {
    // NSURLSession uses HTTP pipelining/keep-alive by default
}

Result Client::Get(const std::string &path) {
    @autoreleasepool {
        NSURL *url = impl_->make_url(path);
        if (!url) {
            Log::log_print(ERR, "apple_httplib: invalid URL: %s%s", impl_->host.c_str(), path.c_str());
            return Result(Error::Connection);
        }

        Log::log_print(DEBUG, "apple_httplib: GET %s", [[url absoluteString] UTF8String]);

        __block int status = 0;
        __block NSData *body_data = nil;
        __block NSError *net_error = nil;

        dispatch_semaphore_t sem = dispatch_semaphore_create(0);

        NSURLSessionDataTask *task =
            [impl_->session dataTaskWithURL:url
                          completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
                            if (error) {
                                net_error = error;
                            } else if ([response isKindOfClass:[NSHTTPURLResponse class]]) {
                                NSHTTPURLResponse *http = (NSHTTPURLResponse *)response;
                                status = (int)http.statusCode;
                                body_data = [data copy];
                            }
                            dispatch_semaphore_signal(sem);
                          }];
        [task resume];
        dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);

        if (net_error) {
            Log::log_print(ERR, "apple_httplib: GET %s%s failed: %s", impl_->host.c_str(), path.c_str(),
                           [[net_error localizedDescription] UTF8String]);
            return Result(Error::Connection);
        }

        Log::log_print(DEBUG, "apple_httplib: GET %s%s => %d (%zu bytes)", impl_->host.c_str(), path.c_str(), status,
                       body_data ? (size_t)body_data.length : 0);

        Response resp;
        resp.status = status;
        if (body_data) {
            resp.body.assign((const char *)body_data.bytes, body_data.length);
        }
        return Result(std::move(resp));
    }
}

Result Client::Get(const std::string &path, std::function<bool(const char *data, size_t len)> chunk_cb) {
    @autoreleasepool {
        NSURL *url = impl_->make_url(path);
        if (!url) {
            Log::log_print(ERR, "apple_httplib: invalid URL (stream): %s%s", impl_->host.c_str(), path.c_str());
            return Result(Error::Connection);
        }

        Log::log_print(DEBUG, "apple_httplib: GET (stream) %s", [[url absoluteString] UTF8String]);

        __block int status = 0;
        __block NSData *body_data = nil;
        __block NSError *net_error = nil;

        dispatch_semaphore_t sem = dispatch_semaphore_create(0);

        NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url];
        request.timeoutInterval = impl_->read_timeout;

        NSURLSessionDataTask *task =
            [impl_->session dataTaskWithRequest:request
                              completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
                                if (error) {
                                    net_error = error;
                                } else if ([response isKindOfClass:[NSHTTPURLResponse class]]) {
                                    NSHTTPURLResponse *http = (NSHTTPURLResponse *)response;
                                    status = (int)http.statusCode;
                                    body_data = [data copy];
                                }
                                dispatch_semaphore_signal(sem);
                              }];
        [task resume];
        dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);

        if (net_error) {
            Log::log_print(ERR, "apple_httplib: GET (stream) %s%s failed: %s", impl_->host.c_str(), path.c_str(),
                           [[net_error localizedDescription] UTF8String]);
            return Result(Error::Connection);
        }

        // Feed received data to chunk callback
        if (body_data && chunk_cb) {
            const size_t chunk_size = 16384;
            const uint8_t *bytes = (const uint8_t *)body_data.bytes;
            size_t remaining = body_data.length;
            size_t offset = 0;
            while (remaining > 0) {
                size_t n = std::min(remaining, chunk_size);
                if (!chunk_cb((const char *)(bytes + offset), n)) {
                    return Result(Error::Canceled);
                }
                offset += n;
                remaining -= n;
            }
        }

        Response resp;
        resp.status = status;
        return Result(std::move(resp));
    }
}

} // namespace httplib
