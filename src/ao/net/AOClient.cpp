#include "AOClient.h"

#include "utils/Log.h"

AOClient::AOClient() : incomplete_buf(""), conn_state(NOT_CONNECTED) {
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
        // Extract the complete message up to the delimiter
        std::string complete_msg = incomplete_buf.substr(0, delimiter_pos + std::strlen(AOPacket::DELIMITER));

        // Remove the processed message and the delimiter from the buffer
        incomplete_buf.erase(0, delimiter_pos + std::strlen(AOPacket::DELIMITER));

        // Process the complete message
        try {
            // Parse the serialized message into an AOPacket
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

void AOClient::add_message(const AOPacket& packet) {
    std::string msg_str = packet.serialize();
    buffered_messages.push_back(msg_str);
}