#include "EngineEventBridge.h"

#include "utils/Log.h"

#include <QAbstractEventDispatcher>
#include <QThread>

EngineEventBridge::EngineEventBridge(QObject* parent) : QObject(parent) {
}

EngineEventBridge::~EngineEventBridge() {
    stop();
}

void EngineEventBridge::add_channel(DrainFn drain) {
    Q_ASSERT_X(!connection_, "EngineEventBridge::add_channel", "Channels must be registered before start()");
    drains_.push_back(std::move(drain));
}

void EngineEventBridge::start() {
    Q_ASSERT_X(!connection_, "EngineEventBridge::start", "Already started");

    auto* dispatcher = QAbstractEventDispatcher::instance(QThread::currentThread());
    Q_ASSERT_X(dispatcher, "EngineEventBridge::start",
               "start() must be called from a thread with a running event loop");

    // DirectConnection: drain_all() runs inline on the dispatcher's thread the
    // moment the event loop wakes for any reason (input, vsync, network, etc.).
    // This matches the SDL frontend's poll-at-top-of-loop ordering and adds no
    // extra wakeups to the system.
    connection_ = connect(dispatcher, &QAbstractEventDispatcher::awake, this, &EngineEventBridge::drain_all,
                          Qt::DirectConnection);
    Log::info("[EngineEventBridge] started with {} drain channels", drains_.size());
}

void EngineEventBridge::stop() {
    Log::info("[EngineEventBridge] stopping");
    disconnect(connection_);
    connection_ = {};
}

void EngineEventBridge::drain_all() {
    for (auto& drain : drains_)
        drain();
}
