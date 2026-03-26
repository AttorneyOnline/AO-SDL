#include "EngineEventBridge.h"

#include <QAbstractEventDispatcher>
#include <QThread>

EngineEventBridge::EngineEventBridge(QObject* parent)
    : QObject(parent) {}

EngineEventBridge::~EngineEventBridge() {
    stop();
}

void EngineEventBridge::addChannel(DrainFn drain) {
    Q_ASSERT_X(!m_connection, "EngineEventBridge::addChannel",
               "Channels must be registered before start()");
    m_drains.push_back(std::move(drain));
}

void EngineEventBridge::start() {
    Q_ASSERT_X(!m_connection, "EngineEventBridge::start", "Already started");

    auto* dispatcher = QAbstractEventDispatcher::instance(QThread::currentThread());
    Q_ASSERT_X(dispatcher, "EngineEventBridge::start",
               "start() must be called from a thread with a running event loop");

    // DirectConnection: drainAll() runs inline on the dispatcher's thread the
    // moment the event loop wakes for any reason (input, vsync, network, etc.).
    // This matches the SDL frontend's poll-at-top-of-loop ordering and adds no
    // extra wakeups to the system.
    m_connection = connect(
        dispatcher, &QAbstractEventDispatcher::awake,
        this, &EngineEventBridge::drainAll,
        Qt::DirectConnection);
}

void EngineEventBridge::stop() {
    disconnect(m_connection);
    m_connection = {};
}

void EngineEventBridge::drainAll() {
    for (auto& drain : m_drains)
        drain();
}
