#pragma once

#include "IQtScreenController.h"
#include "ui/models/CharListModel.h"

#include <QAbstractListModel>
#include <QObject>

#include <string>
#include <vector>

class CourtroomController;
class UIManager;

/**
 * @brief Qt controller for the character selection screen.
 *
 * drain() consumes CharacterListEvent, CharsCheckEvent, and UIEvent directly
 * from EventManager.  No CharSelectScreen is needed as a data source.
 *
 * On UIEvent::ENTERED_COURTROOM the controller:
 *   1. Injects the character name into CourtroomController.
 *   2. Pushes CourtroomScreen onto UIManager's stack.
 *
 * The character list is fed into the QML model in batches
 * (kHydrateBatch entries per tick) so the UI hydrates progressively
 * rather than stalling on a single frame.  Icon prefetch and resolve
 * are similarly rate-limited.
 */
class CharSelectController : public IQtScreenController {
    Q_OBJECT
    Q_PROPERTY(CharListModel* model READ model CONSTANT)

  public:
    explicit CharSelectController(UIManager& uiMgr, CourtroomController& crCtrl, QObject* parent = nullptr);

    /// IQtScreenController
    void drain() override;

    CharListModel* model() {
        return &m_model;
    }

    /// Select the character at index.  Publishes CharSelectRequestEvent.
    Q_INVOKABLE void selectCharacter(int index);

    /// Return to the server list.
    Q_INVOKABLE void disconnect();

  private:
    struct CharEntry {
        std::string folder;
        bool taken = false;
        bool iconResolved = false;
    };

    void hydrateModel();
    void prefetchIcons();
    void resolveIcons();

    UIManager& m_uiMgr;
    CourtroomController& m_crCtrl;
    CharListModel m_model;

    std::vector<CharEntry> m_chars;
    int m_selected = -1;
    int m_hydrateCursor = 0;
    int m_prefetchCursor = 0;
    int m_retryCursor = 0;

    /// Entries inserted into the model per tick during initial hydration.
    static constexpr int kHydrateBatch = 64;
    /// Icon resolves (dataChanged signals) per tick.
    static constexpr int kResolveBatch = 32;
};
