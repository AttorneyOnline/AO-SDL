#include "net/nx/NXEndpoint.h"

#include "game/ACLFlags.h"
#include "game/ASNReputationManager.h"
#include "game/BanManager.h"
#include "game/DatabaseManager.h"
#include "game/FirewallManager.h"
#include "game/GameRoom.h"
#include "game/ServerSession.h"
#include "moderation/ContentModerator.h"
#include "net/EndpointRegistrar.h"

#include <json.hpp>

namespace {

std::optional<RestResponse> require_super(const RestRequest& req) {
    if (!req.session || !req.session->moderator)
        return RestResponse::error(403, "Authentication required");
    if (has_permission(acl_permissions_for_role(req.session->acl_role), ACLPermission::SUPER))
        return std::nullopt;
    return RestResponse::error(403, "SUPER privileges required");
}

// =========================================================================
// GET /admin/bans
// =========================================================================
class AdminBansEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "GET";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/admin/bans";
        return p;
    }
    bool requires_auth() const override {
        return true;
    }
    bool readonly() const override {
        return true;
    }
    CorsPolicy cors_policy() const override {
        return CorsPolicy::Restricted;
    }

    RestResponse handle(const RestRequest& req) override {
        if (auto err = require_super(req))
            return std::move(*err);

        auto* bm = room().ban_manager();
        if (!bm)
            return RestResponse::error(503, "Ban manager unavailable");

        auto query_it = req.query_params.find("query");
        int limit = 50;
        auto limit_it = req.query_params.find("limit");
        if (limit_it != req.query_params.end()) {
            try {
                limit = std::stoi(limit_it->second);
            } catch (...) {
            }
        }
        limit = std::clamp(limit, 1, 500);

        std::vector<BanEntry> bans;
        if (query_it != req.query_params.end() && !query_it->second.empty())
            bans = bm->search_bans(query_it->second, limit);
        else
            bans = bm->list_bans(limit);

        nlohmann::json arr = nlohmann::json::array();
        for (const auto& b : bans) {
            arr.push_back({
                {"id", b.id},
                {"ipid", b.ipid},
                {"hdid", b.hdid},
                {"reason", b.reason},
                {"moderator", b.moderator},
                {"timestamp", b.timestamp},
                {"duration", b.duration},
                {"permanent", b.is_permanent()},
            });
        }
        return RestResponse::json(200, {{"bans", std::move(arr)}, {"count", bans.size()}});
    }
};

// =========================================================================
// GET /admin/users
// =========================================================================
class AdminUsersEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "GET";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/admin/users";
        return p;
    }
    bool requires_auth() const override {
        return true;
    }
    bool readonly() const override {
        return true;
    }
    CorsPolicy cors_policy() const override {
        return CorsPolicy::Restricted;
    }

    RestResponse handle(const RestRequest& req) override {
        if (auto err = require_super(req))
            return std::move(*err);

        auto* db = room().db_manager();
        if (!db || !db->is_open())
            return RestResponse::error(503, "Database unavailable");

        auto usernames = db->list_users().get();
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& name : usernames) {
            auto user = db->get_user(name).get();
            if (user)
                arr.push_back({{"username", user->username}, {"acl", user->acl}});
        }
        return RestResponse::json(200, {{"users", std::move(arr)}});
    }
};

// =========================================================================
// GET /admin/moderation-events
// =========================================================================
class AdminModerationEventsEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "GET";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/admin/moderation-events";
        return p;
    }
    bool requires_auth() const override {
        return true;
    }
    bool readonly() const override {
        return true;
    }
    CorsPolicy cors_policy() const override {
        return CorsPolicy::Restricted;
    }

    RestResponse handle(const RestRequest& req) override {
        if (auto err = require_super(req))
            return std::move(*err);

        auto* db = room().db_manager();
        if (!db || !db->is_open())
            return RestResponse::error(503, "Database unavailable");

        ModerationQuery query;
        auto get_param = [&](const char* name) -> std::optional<std::string> {
            auto it = req.query_params.find(name);
            if (it != req.query_params.end() && !it->second.empty())
                return it->second;
            return std::nullopt;
        };

        // Parse helpers that return an error response on bad input rather
        // than throwing — otherwise a bogus `?since=abc` propagates a
        // stoll exception and fails the whole request with a generic 500.
        auto parse_int64 = [](const std::string& s, int64_t& out) -> bool {
            try {
                size_t idx = 0;
                out = std::stoll(s, &idx);
                return idx == s.size();
            }
            catch (...) {
                return false;
            }
        };
        auto parse_int = [](const std::string& s, int& out) -> bool {
            try {
                size_t idx = 0;
                out = std::stoi(s, &idx);
                return idx == s.size();
            }
            catch (...) {
                return false;
            }
        };

        query.ipid = get_param("ipid");
        query.channel = get_param("channel");
        query.action = get_param("action");
        if (auto s = get_param("since")) {
            int64_t v = 0;
            if (!parse_int64(*s, v))
                return RestResponse::error(400, "Invalid 'since' parameter");
            query.since_ms = v;
        }
        if (auto u = get_param("until")) {
            int64_t v = 0;
            if (!parse_int64(*u, v))
                return RestResponse::error(400, "Invalid 'until' parameter");
            query.until_ms = v;
        }
        if (auto l = get_param("limit")) {
            int v = 0;
            if (!parse_int(*l, v))
                return RestResponse::error(400, "Invalid 'limit' parameter");
            query.limit = std::clamp(v, 1, 1000);
        }

        auto events = db->query_moderation_events(std::move(query)).get();
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& e : events) {
            arr.push_back({
                {"timestamp_ms", e.timestamp_ms},
                {"ipid", e.ipid},
                {"channel", e.channel},
                {"message_sample", e.message_sample},
                {"action", e.action},
                {"heat_after", e.heat_after},
                {"reason", e.reason},
            });
        }
        return RestResponse::json(200, {{"events", std::move(arr)}, {"count", events.size()}});
    }
};

// =========================================================================
// GET /admin/mutes
// =========================================================================
class AdminMutesEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "GET";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/admin/mutes";
        return p;
    }
    bool requires_auth() const override {
        return true;
    }
    bool readonly() const override {
        return true;
    }
    CorsPolicy cors_policy() const override {
        return CorsPolicy::Restricted;
    }

    RestResponse handle(const RestRequest& req) override {
        if (auto err = require_super(req))
            return std::move(*err);

        auto* db = room().db_manager();
        if (!db || !db->is_open())
            return RestResponse::error(503, "Database unavailable");

        auto mutes = db->active_mutes().get();
        auto now = std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();

        nlohmann::json arr = nlohmann::json::array();
        for (const auto& m : mutes) {
            int64_t remaining = m.expires_at > 0 ? std::max(int64_t{0}, m.expires_at - now) : -1;
            arr.push_back({
                {"ipid", m.ipid},
                {"reason", m.reason},
                {"moderator", m.moderator},
                {"started_at", m.started_at},
                {"expires_at", m.expires_at},
                {"seconds_remaining", remaining},
            });
        }
        return RestResponse::json(200, {{"mutes", std::move(arr)}});
    }
};

// =========================================================================
// GET /admin/firewall
// =========================================================================
class AdminFirewallEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "GET";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/admin/firewall";
        return p;
    }
    bool requires_auth() const override {
        return true;
    }
    bool readonly() const override {
        return true;
    }
    CorsPolicy cors_policy() const override {
        return CorsPolicy::Restricted;
    }

    RestResponse handle(const RestRequest& req) override {
        if (auto err = require_super(req))
            return std::move(*err);

        auto* fw = room().firewall();
        nlohmann::json rules = nlohmann::json::array();
        bool enabled = false;

        if (fw) {
            enabled = fw->is_enabled();
            for (const auto& r : fw->list_rules()) {
                rules.push_back({
                    {"target", r.target},
                    {"reason", r.reason},
                    {"installed_at", r.installed_at},
                    {"expires_at", r.expires_at},
                });
            }
        }

        return RestResponse::json(200, {{"enabled", enabled}, {"rules", std::move(rules)}});
    }
};

// =========================================================================
// GET /admin/asn-reputation
// =========================================================================
class AdminASNReputationEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "GET";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/admin/asn-reputation";
        return p;
    }
    bool requires_auth() const override {
        return true;
    }
    bool readonly() const override {
        return true;
    }
    CorsPolicy cors_policy() const override {
        return CorsPolicy::Restricted;
    }

    RestResponse handle(const RestRequest& req) override {
        if (auto err = require_super(req))
            return std::move(*err);

        auto* asn = room().asn_reputation();
        nlohmann::json arr = nlohmann::json::array();

        if (asn) {
            for (const auto& e : asn->list_flagged()) {
                std::string status_str;
                switch (e.status) {
                case ASNReputationEntry::Status::NORMAL:
                    status_str = "normal";
                    break;
                case ASNReputationEntry::Status::WATCHED:
                    status_str = "watched";
                    break;
                case ASNReputationEntry::Status::RATE_LIMITED:
                    status_str = "rate_limited";
                    break;
                case ASNReputationEntry::Status::BLOCKED:
                    status_str = "blocked";
                    break;
                }
                arr.push_back({
                    {"asn", e.asn},
                    {"as_org", e.as_org},
                    {"status", status_str},
                    {"total_abuse_events", e.total_abuse_events},
                    {"abusive_ips", e.abusive_ips_in_window.size()},
                    {"block_expires_at", e.block_expires_at},
                    {"block_reason", e.block_reason},
                });
            }
        }

        return RestResponse::json(200, {{"asn_entries", std::move(arr)}});
    }
};

// =========================================================================
// GET /admin/content
// =========================================================================
class AdminContentGetEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "GET";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/admin/content";
        return p;
    }
    bool requires_auth() const override {
        return true;
    }
    bool readonly() const override {
        return true;
    }
    CorsPolicy cors_policy() const override {
        return CorsPolicy::Restricted;
    }

    RestResponse handle(const RestRequest& req) override {
        if (auto err = require_super(req))
            return std::move(*err);

        auto& r = room();
        return RestResponse::json(200, {
                                           {"characters", r.characters},
                                           {"music", r.music},
                                           {"areas", r.areas},
                                       });
    }
};

// =========================================================================
// Registration
// =========================================================================
EndpointRegistrar reg_bans("GET /aonx/v1/admin/bans", [] { return std::make_unique<AdminBansEndpoint>(); });
EndpointRegistrar reg_users("GET /aonx/v1/admin/users", [] { return std::make_unique<AdminUsersEndpoint>(); });
EndpointRegistrar reg_mod_events("GET /aonx/v1/admin/moderation-events",
                                 [] { return std::make_unique<AdminModerationEventsEndpoint>(); });
EndpointRegistrar reg_mutes("GET /aonx/v1/admin/mutes", [] { return std::make_unique<AdminMutesEndpoint>(); });
EndpointRegistrar reg_firewall("GET /aonx/v1/admin/firewall", [] { return std::make_unique<AdminFirewallEndpoint>(); });
EndpointRegistrar reg_asn("GET /aonx/v1/admin/asn-reputation",
                           [] { return std::make_unique<AdminASNReputationEndpoint>(); });
EndpointRegistrar reg_content("GET /aonx/v1/admin/content",
                              [] { return std::make_unique<AdminContentGetEndpoint>(); });

} // namespace

void nx_ep_admin_data() {
}
