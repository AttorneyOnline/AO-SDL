#include "net/nx/NXEndpoint.h"

#include "net/EndpointRegistrar.h"
#include "utils/Version.h"

namespace {

class ServerInfoEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "GET";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/server/info";
        return p;
    }
    bool requires_auth() const override {
        return false;
    }

    RestResponse handle(const RestRequest& /*req*/) override {
        return RestResponse::json(200, {
                                           {"software", "kagami"},
                                           {"version", ao_sdl_version()},
                                           {"name", room().server_name},
                                           {"description", room().server_description},
                                       });
    }
};

EndpointRegistrar reg("GET /aonx/v1/server/info", [] { return std::make_unique<ServerInfoEndpoint>(); });

} // namespace

void nx_ep_server_info() {
}
