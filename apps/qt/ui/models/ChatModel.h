#pragma once

#include <QAbstractListModel>
#include <QString>

#include <vector>

/**
 * @brief QML model for the OOC chat log.
 *
 * Populated by CourtroomController when CT (chat) events arrive.
 * Roles: sender, message, isSystem.
 */
class ChatModel : public QAbstractListModel {
    Q_OBJECT

  public:
    enum Role {
        SenderRole   = Qt::UserRole + 1,
        MessageRole,
        IsSystemRole,
    };

    struct ChatLine {
        QString sender;
        QString message;
        bool    isSystem = false;
    };

    explicit ChatModel(QObject* parent = nullptr);

    /// Append a single chat line and notify QML.
    void appendLine(const ChatLine& line);

    /// Clear all lines (e.g. on disconnect).
    void clear();

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

  private:
    std::vector<ChatLine> m_lines;
};
