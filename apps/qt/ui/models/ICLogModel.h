#pragma once

#include <QAbstractListModel>
#include <QString>

#include <vector>

/**
 * @brief QML model for the IC (in-character) message log.
 *
 * Populated by ICController when ICLogEvent arrives (a message starts playing).
 * Roles: showname, message, colorIdx.
 *
 * colorIdx maps to AO2 text-color indices:
 *   0 white  1 green  2 red  3 orange  4 blue  5 yellow  6 magenta  7 pink
 */
class ICLogModel : public QAbstractListModel {
    Q_OBJECT

  public:
    enum Role {
        ShownameRole = Qt::UserRole + 1,
        MessageRole,
        ColorIdxRole,
    };

    struct Entry {
        QString showname;
        QString message;
        int colorIdx = 0;
    };

    explicit ICLogModel(QObject* parent = nullptr);

    void appendEntry(const Entry& entry);
    void clear();

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

  private:
    static constexpr int kMaxEntries = 200;

    std::vector<Entry> m_entries;
};
