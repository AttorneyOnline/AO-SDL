#pragma once

#include "IQtScreenController.h"
#include "ui/models/ChatModel.h"

#include <QObject>
#include <QString>

/**
 * @brief Qt controller for OOC (out-of-character) chat.
 *
 * drain() consumes ChatEvent from EventManager and appends lines to ChatModel.
 * sendOOCMessage() publishes OutgoingChatEvent to the engine.
 *
 * The oocName used as the sender is supplied by the caller (typically the
 * character name from ICController via CourtroomScreen).
 */
class ChatController : public IQtScreenController {
    Q_OBJECT
    Q_PROPERTY(ChatModel* chatModel READ chatModel CONSTANT)

  public:
    explicit ChatController(QObject* parent = nullptr);

    void drain() override;

    ChatModel* chatModel() {
        return &m_chat;
    }

    /**
     * @brief Publish an OutgoingChatEvent.
     * @param name    Sender display name (typically the character name).
     * @param message OOC message text.
     */
    Q_INVOKABLE void sendOOCMessage(const QString& name, const QString& message);

    void reset();

  private:
    ChatModel m_chat;
};
