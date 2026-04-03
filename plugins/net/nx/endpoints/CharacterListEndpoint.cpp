#include "net/nx/NXEndpoint.h"

#include "net/EndpointRegistrar.h"

namespace {

class CharacterListEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "GET";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/characters";
        return p;
    }
    bool requires_auth() const override {
        return true;
    }
    bool readonly() const override {
        return true;
    }

    RestResponse handle(const RestRequest& /*req*/) override {
        auto& chars = room().characters;
        auto& taken = room().char_taken;

        nlohmann::json list = nlohmann::json::array();
        for (int i = 0; i < static_cast<int>(chars.size()); ++i) {
            list.push_back({
                {"char_id", room().char_id_at(i)},
                {"available", i < static_cast<int>(taken.size()) && taken[i] == 0},
            });
        }

        return RestResponse::json(200, {{"characters", std::move(list)}});
    }
};

EndpointRegistrar reg("GET /aonx/v1/characters", [] { return std::make_unique<CharacterListEndpoint>(); });

} // namespace

void nx_ep_character_list() {
}
