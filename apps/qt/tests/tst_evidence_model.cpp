#include <QTest>
#include <QSignalSpy>

#include "EvidenceModel.h"
#include "event/EvidenceListEvent.h"

class TestEvidenceModel : public QObject {
    Q_OBJECT

  private:
    static EvidenceItem make(const char* name, const char* desc, const char* img)
    {
        EvidenceItem e;
        e.name = name;
        e.description = desc;
        e.image = img;
        return e;
    }

  private slots:
    void initial_rowCount_isZero()
    {
        EvidenceModel model;
        QCOMPARE(model.rowCount(), 0);
    }

    void reset_populatesRows()
    {
        EvidenceModel model;
        model.reset({make("Knife", "A sharp knife", "knife.png"),
                     make("Photo", "A photograph", "photo.png")});
        QCOMPARE(model.rowCount(), 2);
    }

    void reset_replacesExistingData()
    {
        EvidenceModel model;
        model.reset({make("Knife", "", "")});
        model.reset({make("Photo", "", ""), make("Note", "", "")});
        QCOMPARE(model.rowCount(), 2);
    }

    void reset_emitsModelReset()
    {
        EvidenceModel model;
        QSignalSpy spy(&model, &QAbstractListModel::modelReset);
        model.reset({make("Knife", "", "")});
        QCOMPARE(spy.count(), 1);
    }

    void clear_resetsToEmpty()
    {
        EvidenceModel model;
        model.reset({make("Knife", "", "")});
        model.clear();
        QCOMPARE(model.rowCount(), 0);
    }

    void clear_emitsModelReset()
    {
        EvidenceModel model;
        model.reset({make("Knife", "", "")});
        QSignalSpy spy(&model, &QAbstractListModel::modelReset);
        model.clear();
        QCOMPARE(spy.count(), 1);
    }

    void data_nameRole()
    {
        EvidenceModel model;
        model.reset({make("Switchblade", "desc", "")});
        QCOMPARE(model.data(model.index(0), EvidenceModel::NameRole).toString(),
                 QStringLiteral("Switchblade"));
    }

    void data_descriptionRole()
    {
        EvidenceModel model;
        model.reset({make("name", "Key evidence", "")});
        QCOMPARE(model.data(model.index(0), EvidenceModel::DescriptionRole).toString(),
                 QStringLiteral("Key evidence"));
    }

    void data_imageRole()
    {
        EvidenceModel model;
        model.reset({make("name", "desc", "evidence/knife.png")});
        QCOMPARE(model.data(model.index(0), EvidenceModel::ImageRole).toString(),
                 QStringLiteral("evidence/knife.png"));
    }

    void data_invalidIndex_returnsInvalid()
    {
        EvidenceModel model;
        QVERIFY(!model.data(model.index(0)).isValid());
    }

    void roleNames_containsExpectedKeys()
    {
        EvidenceModel model;
        const auto names = model.roleNames();
        QVERIFY(names.contains(EvidenceModel::NameRole));
        QVERIFY(names.contains(EvidenceModel::DescriptionRole));
        QVERIFY(names.contains(EvidenceModel::ImageRole));
    }

    void reset_toEmpty_clears()
    {
        EvidenceModel model;
        model.reset({make("Knife", "", "")});
        model.reset({});
        QCOMPARE(model.rowCount(), 0);
    }
};

QTEST_MAIN(TestEvidenceModel)
#include "tst_evidence_model.moc"
