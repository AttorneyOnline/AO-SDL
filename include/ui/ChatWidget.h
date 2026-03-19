/**
 * @file ChatWidget.h
 * @brief Backend-agnostic chat state and event handling.
 */
#pragma once

#include <string>

/**
 * @brief Backend-agnostic chat widget holding message state and input buffers.
 *
 * ChatWidget owns the chat log buffer and user input fields. Rendering is
 * handled by the active UI backend, which reads state via the const accessors
 * and writes user input back via the mutable buffer accessors.
 *
 * @note send_message() publishes an OutgoingChatEvent via the event system.
 *       handle_events() consumes incoming ChatEvent instances.
 */
class ChatWidget {
  public:
    /**
     * @brief Process pending chat events for this frame.
     *
     * Consumes incoming ChatEvent instances and appends them to the buffer.
     */
    void handle_events();

    /**
     * @brief Get the accumulated chat log.
     * @return Const reference to the chat log string.
     */
    const std::string& get_buffer() const { return m_buffer; }

    /**
     * @brief Get the current display name.
     * @return Null-terminated C string containing the name.
     */
    const char* get_name() const { return m_name; }

    /**
     * @brief Get the current message text.
     * @return Null-terminated C string containing the message.
     */
    const char* get_message() const { return m_message; }

    /**
     * @brief Get a mutable pointer to the name input buffer.
     *
     * UI backends use this to write user input directly into the buffer
     * (e.g. with ImGui::InputText).
     *
     * @return Mutable pointer to the name buffer.
     */
    char* name_buf() { return m_name; }

    /**
     * @brief Get the size of the name input buffer.
     * @return Size in bytes of the name buffer (including null terminator space).
     */
    size_t name_buf_size() const { return sizeof(m_name); }

    /**
     * @brief Get a mutable pointer to the message input buffer.
     *
     * UI backends use this to write user input directly into the buffer.
     *
     * @return Mutable pointer to the message buffer.
     */
    char* message_buf() { return m_message; }

    /**
     * @brief Get the size of the message input buffer.
     * @return Size in bytes of the message buffer (including null terminator space).
     */
    size_t message_buf_size() const { return sizeof(m_message); }

    /**
     * @brief Send the current message.
     *
     * Publishes an OutgoingChatEvent containing the current name and message,
     * then clears the message buffer.
     */
    void send_message();

  private:
    std::string m_buffer;        ///< Accumulated chat log.
    char m_name[32] = "";        ///< User name input buffer.
    char m_message[1024] = "";   ///< Message input buffer.
};
