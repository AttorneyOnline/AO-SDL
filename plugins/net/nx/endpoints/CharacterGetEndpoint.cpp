#include "net/nx/NXEndpoint.h"

#include "net/EndpointRegistrar.h"

namespace {

class CharacterGetEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "GET";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/characters/:char_id";
        return p;
    }
    bool requires_auth() const override {
        return true;
    }
    bool readonly() const override {
        return true;
    }

    RestResponse handle(const RestRequest& req) override {
        auto it = req.path_params.find("char_id");
        if (it == req.path_params.end())
            return RestResponse::error(400, "Missing char_id");

        int index = room().find_char_index(it->second);
        if (index < 0)
            return RestResponse::error(404, "Character not found");

        const auto& name = room().characters[index];

        // Stub manifest — full emote/asset data comes in Phase 7.
        return RestResponse::json(200, {
                                           {"schema", "character"},
                                           {"schema_version", 1},
                                           {"id", it->second},
                                           {"name", name},
                                           {"icon", ""},
                                           {"defaults", {{"side", "wit"}, {"blip", ""}}},
                                           {"emotes", nlohmann::json::array()},
                                       });
    }
};

EndpointRegistrar reg("GET /aonx/v1/characters/:char_id", [] { return std::make_unique<CharacterGetEndpoint>(); });

} // namespace

void nx_ep_character_get() {
}
