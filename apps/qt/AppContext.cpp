#include "AppContext.h"

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
