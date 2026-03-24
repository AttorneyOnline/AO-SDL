/**
 * Ad-hoc test for the apple_httplib shim.
 * Compile and run on macOS to verify NSURLSession-based HTTP works.
 *
 * Build:
 *   clang++ -std=c++20 -ObjC++ -framework Foundation \
 *       -I../../../include test_httplib.mm apple_httplib.mm \
 *       -o test_httplib && ./test_httplib
 */
#import "apple_httplib.h"
#import <cstdio>

// Stub Log so apple_httplib.mm compiles without the engine
namespace Log {
void log_print(int, const char *, ...) {}
} // namespace Log

int main() {
    int passed = 0, failed = 0;

    auto check = [&](const char *name, bool ok) {
        if (ok) {
            printf("  PASS: %s\n", name);
            passed++;
        } else {
            printf("  FAIL: %s\n", name);
            failed++;
        }
    };

    // Test 1: HTTP GET (plain)
    printf("Test 1: HTTP GET http://servers.aceattorneyonline.com/servers\n");
    {
        httplib::Client cli("http://servers.aceattorneyonline.com");
        cli.set_connection_timeout(5);
        cli.set_read_timeout(10);
        auto res = cli.Get("/servers");
        check("result is truthy", (bool)res);
        if (res) {
            check("status == 200", res->status == 200);
            check("body is non-empty", !res->body.empty());
            check("body starts with [", res->body.size() > 0 && res->body[0] == '[');
            printf("    body size: %zu bytes\n", res->body.size());
        }
    }

    // Test 2: HTTPS GET
    printf("Test 2: HTTPS GET https://attorneyoffline.de/base/extensions.json\n");
    {
        httplib::Client cli("https://attorneyoffline.de");
        cli.set_connection_timeout(5);
        cli.set_read_timeout(10);
        auto res = cli.Get("/base/extensions.json");
        check("result is truthy", (bool)res);
        if (res) {
            check("status == 200", res->status == 200);
            check("body is non-empty", !res->body.empty());
            printf("    body size: %zu bytes\n", res->body.size());
            printf("    body preview: %.100s\n", res->body.c_str());
        }
    }

    // Test 3: HTTPS streaming GET
    printf("Test 3: HTTPS streaming GET\n");
    {
        httplib::Client cli("https://attorneyoffline.de");
        cli.set_connection_timeout(5);
        cli.set_read_timeout(10);
        size_t total_bytes = 0;
        int chunk_count = 0;
        auto res = cli.Get("/base/extensions.json", [&](const char *data, size_t len) -> bool {
            total_bytes += len;
            chunk_count++;
            return true;
        });
        check("result is truthy", (bool)res);
        if (res) {
            check("status == 200", res->status == 200);
            check("received chunks", chunk_count > 0);
            check("received bytes", total_bytes > 0);
            printf("    chunks: %d, total: %zu bytes\n", chunk_count, total_bytes);
        }
    }

    // Test 4: 404 handling
    printf("Test 4: 404 response\n");
    {
        httplib::Client cli("https://attorneyoffline.de");
        cli.set_connection_timeout(5);
        cli.set_read_timeout(10);
        auto res = cli.Get("/base/nonexistent_file_12345.xyz");
        check("result is truthy", (bool)res);
        if (res) {
            check("status == 404", res->status == 404);
        }
    }

    // Test 5: Connection error (bad host)
    printf("Test 5: Connection error\n");
    {
        httplib::Client cli("http://this-host-does-not-exist.invalid");
        cli.set_connection_timeout(2);
        cli.set_read_timeout(2);
        auto res = cli.Get("/");
        check("result is falsy", !res);
        check("error is Connection", res.error() == httplib::Error::Connection);
    }

    printf("\n%d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
