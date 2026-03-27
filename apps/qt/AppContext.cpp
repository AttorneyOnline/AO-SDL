#include "AppContext.h"

#include "ui/UIManager.h"
#include "ui/Screen.h"

AppContext::AppContext(QObject* parent)
    : QObject(parent) {}

AppContext& AppContext::instance() {
    static AppContext ctx;
    return ctx;
}

void AppContext::setControllers(ServerListController* sl,
                                CharSelectController* cs,
                                CourtroomController*  cr) {
    m_sl = sl;
    m_cs = cs;
    m_cr = cr;
}

void AppContext::setDummyController(DummyController* d) {
    m_dummy = d;
}

void AppContext::setUIManager(UIManager* mgr) {
    m_uiMgr = mgr;
}

void AppContext::syncCurrentScreenId() {
    if (!m_uiMgr)
        return;

    Screen* screen = m_uiMgr->active_screen();
    QString newId  = screen ? QString::fromStdString(screen->screen_id()) : QString{};

    if (newId == m_currentScreenId)
        return;

    m_currentScreenId = newId;
    emit currentScreenIdChanged();
}
