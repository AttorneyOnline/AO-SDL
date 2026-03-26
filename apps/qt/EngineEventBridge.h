#pragma once

#include <QMetaObject>
#include <QObject>
#include <functional>
#include <vector>

/**
 * @brief Bridges the engine's pull-based EventManager into the Qt event loop.
 *
 * Connects to QAbstractEventDispatcher::awake() with Qt::DirectConnection so
 * that every registered drain callback is called inline on each event loop
 * wake-up. This gives the same per-iteration polling cadence as the SDL
 * frontend's SDL_PollEvent loop without introducing a fixed-interval timer.
 *
 * Single responsibility: dispatcher connection lifecycle and drain dispatch.
 * Has no knowledge of specific event types, QML models, or controllers.
 *
 * Usage:
 * @code
 *   EngineEventBridge bridge;
 *   bridge.addChannel([&] {
 *       auto& ch = EventManager::instance().get_channel<FooEvent>();
 *       while (auto ev = ch.get_event()) { ... }
 *   });
 *   bridge.start();   // connect to current thread's dispatcher
 *   app.exec();
 *   bridge.stop();
 * @endcode
 */
class EngineEventBridge final : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(EngineEventBridge)

  public:
    using DrainFn = std::function<void()>;

    explicit EngineEventBridge(QObject* parent = nullptr);
    ~EngineEventBridge() override;

    /**
     * @brief Register a drain callback invoked on every event loop wake-up.
     *
     * Callbacks are called in registration order. Must be registered before
     * start() — registering after start() is not thread-safe.
     *
     * @param drain Function to call each iteration. Must not block.
     */
    void addChannel(DrainFn drain);

    /**
     * @brief Connect to the calling thread's event dispatcher.
     *
     * Must be called after QCoreApplication exists and from the thread that
     * owns this object. Calling start() twice is an error.
     */
    void start();

    /**
     * @brief Disconnect from the dispatcher. Idempotent and thread-safe.
     */
    void stop();

  private:
    void drainAll();

    std::vector<DrainFn>    m_drains;
    QMetaObject::Connection m_connection;
};
