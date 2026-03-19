/**
 * @file OutgoingChatEvent.h
 * @brief Event representing a chat message to be sent to the server.
 * @ingroup events
 */
#pragma once

#include "Event.h"

#include <string>

/**
 * @brief A chat message that the client wants to send to the server.
 * @ingroup events
 */
class OutgoingChatEvent : public Event {
  public:
    /**
     * @brief Constructs an OutgoingChatEvent.
     * @param sender_name Display name of the local sender.
     * @param message     The chat message body to send.
     */
    OutgoingChatEvent(std::string sender_name, std::string message);

    /**
     * @brief Returns a human-readable representation of the outgoing chat event.
     * @return String describing sender and message.
     */
    std::string to_string() const override;

    /**
     * @brief Gets the sender's display name.
     * @return The sender name.
     */
    std::string get_sender_name();

    /**
     * @brief Gets the chat message body.
     * @return The message text to send.
     */
    std::string get_message();

  private:
    std::string sender_name; /**< Display name of the sender. */
    std::string message;     /**< Chat message body. */
};
