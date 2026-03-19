/**
 * @file ChatEvent.h
 * @brief Event representing an incoming chat message received from the server.
 * @ingroup events
 */
#pragma once

#include "Event.h"

#include <string>

/**
 * @brief An incoming chat message received from the server.
 * @ingroup events
 */
class ChatEvent : public Event {
  public:
    /**
     * @brief Constructs a ChatEvent.
     * @param sender_name Display name of the message sender.
     * @param message     The chat message body.
     * @param system_message @c true if this is a system/server message rather
     *                       than a player message.
     */
    ChatEvent(std::string sender_name, std::string message, bool system_message);

    /**
     * @brief Returns a human-readable representation of the chat event.
     * @return String describing sender, message, and system flag.
     */
    std::string to_string() const override;

    /**
     * @brief Gets the sender's display name.
     * @return The sender name.
     */
    std::string get_sender_name();

    /**
     * @brief Gets the chat message body.
     * @return The message text.
     */
    std::string get_message();

    /**
     * @brief Indicates whether this is a system message.
     * @return @c true if the message originated from the server/system.
     */
    bool get_system_message();

  private:
    std::string sender_name; /**< Display name of the sender. */
    std::string message;     /**< Chat message body. */
    bool system_message;     /**< Whether this is a system message. */
};
