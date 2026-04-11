#pragma once

#include "IQtScreenController.h"
#include "ui/models/EvidenceModel.h"

#include <QObject>

/**
 * @brief Qt controller for the evidence list.
 *
 * drain() consumes EvidenceListEvent and resets the EvidenceModel.
 */
class EvidenceController : public IQtScreenController {
    Q_OBJECT
    Q_PROPERTY(EvidenceModel* evidenceModel READ evidenceModel CONSTANT)
    Q_PROPERTY(int selectedIndex READ selectedIndex NOTIFY selectedIndexChanged)

  public:
    explicit EvidenceController(QObject* parent = nullptr);

    void drain() override;

    EvidenceModel* evidenceModel() {
        return &m_evidence;
    }

    int selectedIndex() const {
        return m_selectedIndex;
    }

    /// Select evidence at index (-1 = none). Called from QML on delegate click.
    Q_INVOKABLE void selectEvidence(int index);

    void reset();

  signals:
    void selectedIndexChanged();

  private:
    EvidenceModel m_evidence;
    int m_selectedIndex = -1;
};
