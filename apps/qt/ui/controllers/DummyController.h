#pragma once

#include "IQtScreenController.h"

class UIManager;

/**
 * @brief Qt controller for DummyScreen.
 *
 * Exposes a single invokable: goBack(), which pops DummyScreen off the
 * UIManager stack and returns to the previous screen (ServerList).
 *
 * No models, no event channels — DummyScreen carries no state.
 */
class DummyController : public IQtScreenController {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(DummyController)

  public:
    explicit DummyController(UIManager& uiMgr, QObject* parent = nullptr);

    void drain() override {}  // DummyScreen has no state to drain.

    Q_INVOKABLE void goBack();

  private:
    UIManager& m_uiMgr;
};
