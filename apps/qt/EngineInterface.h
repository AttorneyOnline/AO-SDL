#pragma once

#include <memory>

class GameThread;
class HttpPool;
class IScenePresenter;
class ProtocolHandler;
class Session;
class StateBuffer;
class WSClientThread;

/**
 * @brief Owns and manages all engine-layer services for the Qt frontend.
 *
 * Pure C++ — no Qt dependency.  Constructed before any Qt objects and
 * destroyed after them.  QtAppInterface receives a reference and wires
 * drain() into the Qt event loop via EngineEventBridge.
 *
 * The SDL frontend keeps equivalent wiring in main() because SDL's event
 * loop is an explicit poll the application owns.  Qt's loop is framework-
 * managed (app.exec()), so engine services are factored into this class
 * to give them a clear lifetime independent of the QObject hierarchy.
 */
class EngineInterface {
  public:
    explicit EngineInterface(StateBuffer& render_buffer);
    ~EngineInterface();

    EngineInterface(const EngineInterface&) = delete;
    EngineInterface& operator=(const EngineInterface&) = delete;

    /**
     * @brief Poll engine services.  Called once per Qt event-loop wakeup.
     *
     * Drains HTTP callbacks and handles session lifecycle events
     * (SessionStartEvent, AssetUrlEvent, SessionEndEvent).
     */
    void drain();

    /**
     * @brief Shut down engine services in reverse construction order.
     *
     * Safe to call multiple times.  Also called by the destructor.
     */
    void stop();

    /// Exposes the HTTP pool so callers (e.g. server-list fetch) can issue requests.
    HttpPool& http();

  private:
    std::unique_ptr<HttpPool> http_;
    std::unique_ptr<ProtocolHandler> protocol_;
    std::unique_ptr<WSClientThread> net_;
    std::unique_ptr<IScenePresenter> presenter_;
    std::unique_ptr<GameThread> game_;
    std::unique_ptr<Session> active_session_;
};
