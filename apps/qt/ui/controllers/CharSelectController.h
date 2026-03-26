#pragma once

#include "IQtScreenController.h"

#include <QAbstractListModel>
#include <QObject>

class CharSelectScreen;

/**
 * @brief Flat list model for the character roster on the char-select screen.
 *
 * Exposes the character folder name and taken status.  Icon loading is done
 * asynchronously by the engine; the Qt side reflects what the Screen reports
 * each frame.
 */
class CharListModel : public QAbstractListModel {
    Q_OBJECT

  public:
    enum Role { NameRole = Qt::UserRole + 1, TakenRole };

    struct CharEntry {
        QString name;
        bool    taken = false;
    };

    explicit CharListModel(QObject* parent = nullptr);

    void reset(const std::vector<CharEntry>& chars);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

  private:
    std::vector<CharEntry> m_entries;
};

// --------------------------------------------------------------------------

/**
 * @brief Qt controller for the character selection screen.
 *
 * Syncs CharListModel from CharSelectScreen::get_chars() and exposes
 * selectCharacter() for QML.
 */
class CharSelectController : public IQtScreenController {
    Q_OBJECT
    Q_PROPERTY(CharListModel* model READ model CONSTANT)

  public:
    explicit CharSelectController(QObject* parent = nullptr);

    void sync(Screen& screen) override;

    CharListModel* model() { return &m_model; }

    /// Select the character at index and send a request to the server.
    Q_INVOKABLE void selectCharacter(int index);

    /// Return to the server list.
    Q_INVOKABLE void disconnect();

  private:
    CharListModel     m_model;
    CharSelectScreen* m_screen = nullptr;
};
