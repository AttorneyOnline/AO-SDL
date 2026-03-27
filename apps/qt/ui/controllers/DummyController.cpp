#include "DummyController.h"

#include "ui/UIManager.h"

DummyController::DummyController(UIManager& uiMgr, QObject* parent)
    : IQtScreenController(parent)
    , m_uiMgr(uiMgr)
{}

void DummyController::goBack() {
    m_uiMgr.pop_screen();
}
