#include "net/nx/NXEndpoint.h"

#include "net/EndpointRegistrar.h"

namespace {

class CharacterSelectEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "POST";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/characters/select";
        return p;
    }
    bool requires_auth() const override {
        return true;
    }

    RestResponse handle(const RestRequest& req) override {
        if (!req.body)
            return RestResponse::error(400, "Request body is required");

        auto char_id = req.body->value("char_id", std::string{});
        if (char_id.empty())
            return RestResponse::error(400, "Missing char_id");

        int index = room().find_char_index(char_id);
        if (index < 0)
            return RestResponse::error(404, "Character not found");

        // handle_char_select is the single source of truth for taken state.
        // After a valid index, false always means the character is taken.
        if (!room().handle_char_select({req.session->client_id, index}))
            return RestResponse::error(409, "Character is already taken");

        return RestResponse::json(200, {{"accepted", true}});
    }
};

EndpointRegistrar reg("POST /aonx/v1/characters/select", [] { return std::make_unique<CharacterSelectEndpoint>(); });

} // namespace

void nx_ep_character_select() {
}
