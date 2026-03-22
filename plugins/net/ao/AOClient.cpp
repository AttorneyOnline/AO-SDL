#include "AOClient.h"

#include "ao/event/OutgoingICMessageEvent.h"
#include "PacketTypes.h"
#include "event/CharSelectRequestEvent.h"
#include "event/EventManager.h"
#include "event/OutgoingChatEvent.h"
#include "event/OutgoingHealthBarEvent.h"
#include "event/OutgoingMusicEvent.h"
#include "platform/HardwareId.h"
#include "utils/Log.h"

AOClient::AOClient() = default;

void AOClient::on_connect() {
    conn_state = CONNECTED;
    last_keepalive_ = std::chrono::steady_clock::now();
}

void AOClient::on_message(const std::string& message) {
    incomplete_buf.insert(incomplete_buf.end(), message.begin(), message.end());

    size_t delimiter_pos;
    while ((delimiter_pos = incomplete_buf.find(AOPacket::DELIMITER)) != std::string::npos) {
        std::string complete_msg = incomplete_buf.substr(0, delimiter_pos + std::strlen(AOPacket::DELIMITER));
        incomplete_buf.erase(0, delimiter_pos + std::strlen(AOPacket::DELIMITER));

        try {
            std::unique_ptr<AOPacket> packet = AOPacket::deserialize(complete_msg);
            if (packet->is_valid()) {
                packet->handle(*this);
            }
            else {
                Log::log_print(ERR, "Failed to parse AOPacket from message: %s", complete_msg.c_str());
            }
        }
        catch (const std::exception& e) {
            Log::log_print(ERR, "Exception while handling message: %s, Error: %s", complete_msg.c_str(), e.what());
        }
    }
}

void AOClient::on_disconnect() {
    conn_state = NOT_CONNECTED;
}

std::vector<std::string> AOClient::flush_outgoing() {
    // Drain any outgoing events before returning buffered protocol messages.
    auto& chat_ev_channel = EventManager::instance().get_channel<OutgoingChatEvent>();
    while (auto optev = chat_ev_channel.get_event()) {
        AOPacketCT chat_packet(optev->get_sender_name(), optev->get_message(), false);
        add_message(chat_packet);
    }

    auto& ic_channel = EventManager::instance().get_channel<OutgoingICMessageEvent>();
    while (auto optev = ic_channel.get_event()) {
        AOPacketMS ms_packet(optev->data());
        add_message(ms_packet);
    }

    auto& music_channel = EventManager::instance().get_channel<OutgoingMusicEvent>();
    while (auto optev = music_channel.get_event()) {
        AOPacketMC mc(optev->name(), player_number, optev->showname());
        add_message(mc);
    }

    auto& char_select_channel = EventManager::instance().get_channel<CharSelectRequestEvent>();
    while (auto optev = char_select_channel.get_event()) {
        AOPacketPW pw("");
        add_message(pw);
        AOPacketCC cc(player_number, optev->get_char_id(), platform::hardware_id());
        add_message(cc);
    }

    auto& hp_channel = EventManager::instance().get_channel<OutgoingHealthBarEvent>();
    while (auto optev = hp_channel.get_event()) {
        AOPacketHP hp({std::to_string(optev->side()), std::to_string(optev->value())});
        add_message(hp);
    }

    // Keepalive: send CH packet periodically to prevent proxy/server timeouts
    if (conn_state == JOINED && char_id >= 0) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_keepalive_).count();
        if (elapsed >= KEEPALIVE_INTERVAL_MS) {
            AOPacketCH ch(char_id);
            add_message(ch);
            last_keepalive_ = now;
        }
    }

    std::vector<std::string> out = std::move(buffered_messages);
    buffered_messages.clear();
    return out;
}

void AOClient::add_message(const AOPacket& packet) {
    buffered_messages.push_back(packet.serialize());
}
