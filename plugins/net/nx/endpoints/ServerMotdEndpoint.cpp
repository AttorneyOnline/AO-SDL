#include "net/EndpointRegistrar.h"
#include "net/nx/NXEndpoint.h"

namespace {

class ServerMotdEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "GET";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/server/motd";
        return p;
    }
    bool requires_auth() const override {
        return false;
    }

    RestResponse handle(const RestRequest& /*req*/) override {
        return RestResponse::json(200, {
                                           {"message", server().motd()},
                                       });
    }
};

EndpointRegistrar reg("GET /aonx/v1/server/motd", [] { return std::make_unique<ServerMotdEndpoint>(); });

} // namespace

// Linker anchor — referenced by nx_register_endpoints().
void nx_ep_server_motd() {
}
