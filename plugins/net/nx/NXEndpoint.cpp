#include "NXEndpoint.h"

#include "game/BanManager.h"
#include "game/ServerSession.h"
#include "utils/Log.h"

#include <chrono>

NXEndpoint::ContentVerdictResult NXEndpoint::apply_content_verdict(ServerSession& session,
                                                                   const moderation::ModerationVerdict& verdict,
                                                                   const char* channel) {
    using moderation::ModerationAction;
    ContentVerdictResult out;
    switch (verdict.action) {
    case ModerationAction::NONE:
    case ModerationAction::LOG:
        out.pass_kind = ContentVerdictPass::Pass;
        return out;

    case ModerationAction::CENSOR:
        out.pass_kind = ContentVerdictPass::Censor;
        return out;

    case ModerationAction::DROP:
    case ModerationAction::MUTE:
        // Don't broadcast, but the session token stays valid — the
        // client can keep sending other requests, mute is enforced
        // per-message by the next check() call.
        Log::log_print(INFO, "NX: %s from ipid=%s suppressed [content]: %s", channel, session.ipid.c_str(),
                       verdict.reason.c_str());
        out.early_return = RestResponse::json(200, {{"accepted", false}, {"reason", "content"}});
        return out;

    case ModerationAction::KICK:
        // KICK: invalidate the session token immediately so the
        // client must re-auth to keep using the server. The connection
        // can come back, but it's interrupted as a clear "you got
        // kicked" signal — same semantics as the AO2 KK packet.
        Log::log_print(WARNING, "NX: kicking ipid=%s [content/%s]: %s", session.ipid.c_str(), channel,
                       verdict.reason.c_str());
        server().destroy_session(session.client_id);
        out.early_return = RestResponse::error(403, "Content moderation: " + verdict.reason);
        return out;

    case ModerationAction::BAN:
    case ModerationAction::PERMA_BAN:
        // BAN/PERMA_BAN: write to the BanManager so the BanCheck
        // middleware on SessionCreateEndpoint rejects future
        // reconnect attempts from this IPID/IP/HDID, AND destroy
        // the active session so the current token is dead. Mirrors
        // the AO2 packet behavior path exactly.
        Log::log_print(WARNING, "NX: banning ipid=%s [content/%s]: %s", session.ipid.c_str(), channel,
                       verdict.reason.c_str());
        if (auto* bm = room().ban_manager()) {
            BanEntry entry;
            entry.ipid = session.ipid;
            entry.ip = session.ip_address;
            entry.hdid = session.hardware_id;
            entry.reason = "[auto] content: " + verdict.reason;
            entry.moderator = "ContentModerator";
            entry.timestamp =
                std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                    .count();
            entry.duration = (verdict.action == ModerationAction::PERMA_BAN) ? -2 : 24 * 60 * 60;
            bm->add_ban(std::move(entry));
        }
        server().destroy_session(session.client_id);
        out.early_return = RestResponse::error(403, "Content moderation: " + verdict.reason);
        return out;
    }
    // Unreachable — switch covers every enumerator. Defensive default.
    out.pass_kind = ContentVerdictPass::Pass;
    return out;
}

// Linker anchors defined in each endpoint TU.
void nx_ep_server();
void nx_ep_session_create();
void nx_ep_session_delete();
void nx_ep_session_renew();

// Phase 3: Character & Area endpoints (#90)
void nx_ep_character_list();
void nx_ep_character_get();
void nx_ep_character_select();
void nx_ep_area_list();
void nx_ep_area_get();
void nx_ep_area_players();

// Phase 4: Chat & Area Join endpoints (#91)
void nx_ep_area_join();
void nx_ep_area_ic();
void nx_ep_area_ooc();

// Auth endpoints
void nx_ep_auth_login();
void nx_ep_auth_logout();

// Moderation
void nx_ep_moderation_actions();

// Admin endpoints
void nx_ep_admin_sessions();
void nx_ep_admin_config();
void nx_ep_admin_data();
void nx_ep_admin_content_put();

// Force all endpoint TUs to link. Same pattern as ao_register_packet_types().
void nx_register_endpoints() {
    nx_ep_server();
    nx_ep_session_create();
    nx_ep_session_delete();
    nx_ep_session_renew();

    nx_ep_character_list();
    nx_ep_character_get();
    nx_ep_character_select();
    nx_ep_area_list();
    nx_ep_area_get();
    nx_ep_area_players();

    nx_ep_area_join();
    nx_ep_area_ic();
    nx_ep_area_ooc();

    nx_ep_auth_login();
    nx_ep_auth_logout();

    nx_ep_moderation_actions();

    nx_ep_admin_sessions();
    nx_ep_admin_config();
    nx_ep_admin_data();
    nx_ep_admin_content_put();
}
