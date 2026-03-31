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

        // Check taken before calling handle_char_select so we can
        // distinguish 409 (taken) from a generic false return.
        if (index < static_cast<int>(room().char_taken.size()) && room().char_taken[index])
            return RestResponse::error(409, "Character is already taken");

        bool accepted = room().handle_char_select({req.session->client_id, index});

        return RestResponse::json(200, {{"accepted", accepted}});
    }
};

EndpointRegistrar reg("POST /aonx/v1/characters/select", [] { return std::make_unique<CharacterSelectEndpoint>(); });

} // namespace

void nx_ep_character_select() {
}
