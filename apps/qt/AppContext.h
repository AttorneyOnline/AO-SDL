#pragma once

#include <QObject>
#include <QString>

#include "ui/controllers/CharSelectController.h"
#include "ui/controllers/CourtroomController.h"
#include "ui/controllers/DummyController.h"
#include "ui/controllers/ServerListController.h"

class UIManager;

/**
 * @brief Singleton context object that exposes Qt screen controllers to QML.
 *
 * Registered as the "app" QML context property by QtGameWindow::setupQml().
 * Populated by main() via setControllers() before the window is constructed,
 * so QML bindings always evaluate against valid pointers.
 *
 * Follows the same ownership pattern as RenderBridge: the singleton lives for
 * the process lifetime; the controllers it points to are owned by main().
 *
 * currentScreenId is a dynamic property driven by UIManager's screen stack.
 * main() must call syncCurrentScreenId() via EngineEventBridge on every event
 * loop wakeup so that QML Loader reacts to navigation changes.
 */
class AppContext : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(AppContext)

    Q_PROPERTY(ServerListController* serverListController READ serverListController CONSTANT)
    Q_PROPERTY(CharSelectController* charSelectController READ charSelectController CONSTANT)
    Q_PROPERTY(CourtroomController*  courtroomController  READ courtroomController  CONSTANT)
    Q_PROPERTY(DummyController*      dummyController      READ dummyController      CONSTANT)
    Q_PROPERTY(QString currentScreenId READ currentScreenId NOTIFY currentScreenIdChanged)

  public:
    static AppContext &instance();

    /**
     * @brief Bind the three screen controllers.
     *
     * Must be called once from main() before QtGameWindow is constructed so
     * that QML bindings evaluate against non-null pointers on first access.
     */
    void setControllers(ServerListController* sl,
                        CharSelectController* cs,
                        CourtroomController*  cr);

    void setDummyController(DummyController* d);

    /** Bind the UIManager so syncCurrentScreenId() can read its stack. */
    void setUIManager(UIManager* mgr);

    /**
     * @brief Reads the active screen id from UIManager and emits
     *        currentScreenIdChanged() if it has changed.
     *
     * Must be called on the main thread (e.g., from EngineEventBridge drain).
     */
    void syncCurrentScreenId();

    ServerListController* serverListController() const { return m_sl; }
    CharSelectController* charSelectController() const { return m_cs; }
    CourtroomController*  courtroomController()  const { return m_cr; }
    DummyController*      dummyController()      const { return m_dummy; }
    QString               currentScreenId()      const { return m_currentScreenId; }

  signals:
    void currentScreenIdChanged();

  private:
    explicit AppContext(QObject* parent = nullptr);

    ServerListController* m_sl    = nullptr;
    CharSelectController* m_cs    = nullptr;
    CourtroomController*  m_cr    = nullptr;
    DummyController*      m_dummy = nullptr;
    UIManager*            m_uiMgr = nullptr;
    QString               m_currentScreenId;
};
