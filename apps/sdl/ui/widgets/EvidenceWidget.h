#pragma once

#include "event/EvidenceListEvent.h"
#include "ui/IWidget.h"

#include <string>
#include <vector>

/// Displays the evidence list (LE packet).
class EvidenceWidget : public IWidget {
  public:
    void handle_events() override;
    void render() override;

  private:
    std::vector<EvidenceItem> items_;
    int selected_ = -1;
};
