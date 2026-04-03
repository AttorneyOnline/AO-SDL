#include "net/nx/NXEndpoint.h"

#include "net/EndpointRegistrar.h"
#include "utils/Version.h"

namespace {

class ServerEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "GET";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/server";
        return p;
    }
    bool requires_auth() const override {
        return false;
    }
    bool readonly() const override {
        return true;
    }

    RestResponse handle(const RestRequest& /*req*/) override {
        return RestResponse::json(200, {
                                           {"software", "kagami"},
                                           {"version", ao_sdl_version()},
                                           {"name", room().server_name},
                                           {"description", room().server_description},
                                           {"motd", server().motd()},
                                           {"online", static_cast<int>(room().session_count())},
                                           {"max", room().max_players},
                                       });
    }
};

EndpointRegistrar reg("GET /aonx/v1/server", [] { return std::make_unique<ServerEndpoint>(); });

} // namespace

// Linker anchor — referenced by nx_register_endpoints().
void nx_ep_server() {
}
