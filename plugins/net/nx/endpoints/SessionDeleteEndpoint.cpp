#include "net/nx/NXEndpoint.h"

#include "net/EndpointRegistrar.h"

namespace {

class SessionDeleteEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "DELETE";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/session";
        return p;
    }
    bool requires_auth() const override {
        return true;
    }

    RestResponse handle(const RestRequest& req) override {
        server().destroy_session(req.session->client_id);
        return RestResponse::no_content();
    }
};

EndpointRegistrar reg("DELETE /aonx/v1/session", [] { return std::make_unique<SessionDeleteEndpoint>(); });

} // namespace

// Linker anchor — referenced by nx_register_endpoints().
void nx_ep_session_delete() {
}
