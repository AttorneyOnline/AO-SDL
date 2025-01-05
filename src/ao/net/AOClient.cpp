#include "AOClient.h"

#include "ao/net/PacketTypes.h"
#include "utils/Log.h"
#include "event/EventManager.h"
#include "event/ChatEvent.h"

AOClient::AOClient() : conn_state(NOT_CONNECTED), incomplete_buf("") {
}

std::vector<std::string> AOClient::get_messages() {
    std::vector<std::string> ready_msgs = buffered_messages;
    buffered_messages.clear();

    return ready_msgs;
}

void AOClient::handle_message(const std::string& message) {
    incomplete_buf.insert(incomplete_buf.end(), message.begin(), message.end());

    size_t delimiter_pos;
    // Process all complete messages in the buffer
    while ((delimiter_pos = incomplete_buf.find(AOPacket::DELIMITER)) != std::string::npos) {
        // Extract the complete message up to (and including) the delimiter
        std::string complete_msg = incomplete_buf.substr(0, delimiter_pos + std::strlen(AOPacket::DELIMITER));

        // Remove the processed message and the delimiter from the buffer
        incomplete_buf.erase(0, delimiter_pos + std::strlen(AOPacket::DELIMITER));

        // Process the complete message
        try {
            // Parse the serialized message into an AOPacket
            std::unique_ptr<AOPacket> packet = AOPacket::deserialize(complete_msg);
            if (packet->is_valid()) {
                // Call the packet handler if it is valid
                packet->handle(*this);
            }
            else {
                // Write a log if valid is false, but generally speaking, we should be throwing an exception before we
                // get here. Need to handle it anyways in case whatever failure path we take doesn't yield an exception
                Log::log_print(ERR, "Failed to parse AOPacket from message: %s", complete_msg.c_str());
            }
        }
        catch (const std::exception& e) {
            // todo: maybe rethrow here? packet exceptions signal protocol errors which may need to be handled
            // separately. We could also add more complex exception handling logic here.
            Log::log_print(ERR, "Exception while handling message: %s, Error: %s", complete_msg.c_str(), e.what());
        }
    }
}

void AOClient::handle_events() {
    // Handle any events sent to the network thread that we care about
    // todo: generalize this a bit more

    // Outgoing OOC chat messages
    auto& chat_ev_channel = EventManager::instance().get_channel<ChatEvent>();
    while (chat_ev_channel.has_events(EventTarget::NETWORK)) {
        auto ev = chat_ev_channel.get_event(EventTarget::NETWORK).value();
        AOPacketCT chat_packet(ev.get_sender_name(), ev.get_message(), ev.get_system_message());
        add_message(chat_packet);
    }
}

void AOClient::add_message(const AOPacket& packet) {
    std::string msg_str = packet.serialize();
    buffered_messages.push_back(msg_str);
}
