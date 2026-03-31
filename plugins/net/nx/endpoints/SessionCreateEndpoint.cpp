#include "net/nx/NXEndpoint.h"

#include "net/EndpointRegistrar.h"
#include "utils/GeneratedSchemas.h"

namespace {

class SessionCreateEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "POST";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/session";
        return p;
    }
    bool requires_auth() const override {
        return false;
    }

    RestResponse handle(const RestRequest& req) override {
        if (room().session_count() >= static_cast<size_t>(room().max_players)) {
            return RestResponse::error(503, "Server is full");
        }

        if (!req.body) {
            return RestResponse::error(400, "Request body is required");
        }

        auto& body = *req.body;

        if (auto err = aonx_request_schema("createSession").validate(body); !err.empty()) {
            return RestResponse::error(400, err);
        }

        auto client_name = body.value("client_name", std::string{});
        auto client_version = body.value("client_version", std::string{});
        auto hdid = body.value("hdid", std::string{});

        // Sanitize: cap lengths and strip control characters.
        auto sanitize = [](std::string& s, size_t max_len) {
            if (s.size() > max_len)
                s.resize(max_len);
            std::erase_if(s, [](unsigned char c) { return c < 0x20; });
        };
        sanitize(client_name, 64);
        sanitize(client_version, 32);
        sanitize(hdid, 128);

        auto token = server().create_session(hdid, client_name, client_version);

        return RestResponse::json(201, {
                                           {"token", token},
                                       });
    }
};

EndpointRegistrar reg("POST /aonx/v1/session", [] { return std::make_unique<SessionCreateEndpoint>(); });

} // namespace

// Linker anchor — referenced by nx_register_endpoints().
void nx_ep_session_create() {
}
