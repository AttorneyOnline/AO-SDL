#include "net/nx/NXEndpoint.h"

#include "net/EndpointRegistrar.h"

namespace {

class ServerPlayersEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "GET";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/server/players";
        return p;
    }
    bool requires_auth() const override {
        return false;
    }

    RestResponse handle(const RestRequest& /*req*/) override {
        return RestResponse::json(200, {
                                           {"online", static_cast<int>(room().session_count())},
                                           {"max", room().max_players},
                                       });
    }
};

EndpointRegistrar reg("GET /aonx/v1/server/players", [] { return std::make_unique<ServerPlayersEndpoint>(); });

} // namespace

// Linker anchor — referenced by nx_register_endpoints().
void nx_ep_server_players() {
}
