#pragma once

#include <QObject>

class CharSelectController;
class CourtroomController;
class ServerListController;

/**
 * @brief Singleton context object that exposes Qt screen controllers to QML.
 *
 * Registered as the "app" QML context property by QtGameWindow::setupQml().
 * Populated by main() via setControllers() before the window is constructed,
 * so QML bindings always evaluate against valid pointers.
 *
 * Follows the same ownership pattern as RenderBridge: the singleton lives for
 * the process lifetime; the controllers it points to are owned by main().
 */
class AppContext : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(AppContext)

    Q_PROPERTY(ServerListController* serverListController READ serverListController CONSTANT)
    Q_PROPERTY(CharSelectController* charSelectController READ charSelectController CONSTANT)
    Q_PROPERTY(CourtroomController*  courtroomController  READ courtroomController  CONSTANT)

  public:
    static AppContext& instance();

    /**
     * @brief Bind the three screen controllers.
     *
     * Must be called once from main() before QtGameWindow is constructed so
     * that QML bindings evaluate against non-null pointers on first access.
     */
    void setControllers(ServerListController* sl,
                        CharSelectController* cs,
                        CourtroomController*  cr);

    ServerListController* serverListController() const { return m_sl; }
    CharSelectController* charSelectController() const { return m_cs; }
    CourtroomController*  courtroomController()  const { return m_cr; }

  private:
    explicit AppContext(QObject* parent = nullptr);

    ServerListController* m_sl = nullptr;
    CharSelectController* m_cs = nullptr;
    CourtroomController*  m_cr = nullptr;
};
