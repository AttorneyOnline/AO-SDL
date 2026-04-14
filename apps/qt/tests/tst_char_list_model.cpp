#include <QTest>
#include <QSignalSpy>

#include "CharListModel.h"

class TestCharListModel : public QObject {
    Q_OBJECT

  private:
    static CharListModel::CharEntry make(const char* name, bool taken = false, const char* icon = "")
    {
        return {QString(name), taken, QString(icon)};
    }

  private slots:
    void initial_rowCount_isZero()
    {
        CharListModel model;
        QCOMPARE(model.rowCount(), 0);
    }

    void appendBatch_increasesRowCount()
    {
        CharListModel model;
        model.appendBatch({make("Phoenix"), make("Edgeworth")});
        QCOMPARE(model.rowCount(), 2);
    }

    void appendBatch_isAdditive()
    {
        CharListModel model;
        model.appendBatch({make("Phoenix")});
        model.appendBatch({make("Edgeworth")});
        QCOMPARE(model.rowCount(), 2);
    }

    void appendBatch_emitsRowsInserted()
    {
        CharListModel model;
        QSignalSpy spy(&model, &QAbstractListModel::rowsInserted);
        model.appendBatch({make("Phoenix"), make("Edgeworth")});
        QCOMPARE(spy.count(), 1);
        // first=0, last=1 for a two-entry batch into an empty model
        QCOMPARE(spy.at(0).at(1).toInt(), 0);
        QCOMPARE(spy.at(0).at(2).toInt(), 1);
    }

    void appendBatch_emptyBatch_doesNotNotify()
    {
        CharListModel model;
        QSignalSpy spy(&model, &QAbstractListModel::rowsInserted);
        model.appendBatch({});
        QCOMPARE(spy.count(), 0);
    }

    void clear_resetsToZeroRows()
    {
        CharListModel model;
        model.appendBatch({make("Phoenix")});
        model.clear();
        QCOMPARE(model.rowCount(), 0);
    }

    void clear_emitsModelReset()
    {
        CharListModel model;
        model.appendBatch({make("Phoenix")});
        QSignalSpy spy(&model, &QAbstractListModel::modelReset);
        model.clear();
        QCOMPARE(spy.count(), 1);
    }

    void setTaken_updatesFlag()
    {
        CharListModel model;
        model.appendBatch({make("Phoenix", false)});
        model.setTaken(0, true);
        QCOMPARE(model.data(model.index(0), CharListModel::TakenRole).toBool(), true);
    }

    void setTaken_false_updatesFlag()
    {
        CharListModel model;
        model.appendBatch({make("Phoenix", true)});
        model.setTaken(0, false);
        QCOMPARE(model.data(model.index(0), CharListModel::TakenRole).toBool(), false);
    }

    void setTaken_emitsDataChanged()
    {
        CharListModel model;
        model.appendBatch({make("Phoenix", false)});
        QSignalSpy spy(&model, &QAbstractListModel::dataChanged);
        model.setTaken(0, true);
        QCOMPARE(spy.count(), 1);
    }

    void setIconSource_updatesSource()
    {
        CharListModel model;
        model.appendBatch({make("Phoenix")});
        model.setIconSource(0, QStringLiteral("image://charicon/Phoenix"));
        QCOMPARE(model.data(model.index(0), CharListModel::IconSourceRole).toString(),
                 QStringLiteral("image://charicon/Phoenix"));
    }

    void setIconSource_emitsDataChanged()
    {
        CharListModel model;
        model.appendBatch({make("Phoenix")});
        QSignalSpy spy(&model, &QAbstractListModel::dataChanged);
        model.setIconSource(0, QStringLiteral("image://charicon/Phoenix"));
        QCOMPARE(spy.count(), 1);
    }

    void data_nameRole()
    {
        CharListModel model;
        model.appendBatch({make("Franziska")});
        QCOMPARE(model.data(model.index(0), CharListModel::NameRole).toString(),
                 QStringLiteral("Franziska"));
    }

    void data_takenRole_defaultFalse()
    {
        CharListModel model;
        model.appendBatch({make("Maya")});
        QCOMPARE(model.data(model.index(0), CharListModel::TakenRole).toBool(), false);
    }

    void data_iconSourceRole_empty()
    {
        CharListModel model;
        model.appendBatch({make("Maya")});
        QCOMPARE(model.data(model.index(0), CharListModel::IconSourceRole).toString(), QString());
    }

    void data_invalidIndex_returnsInvalid()
    {
        CharListModel model;
        QVERIFY(!model.data(model.index(0)).isValid());
    }

    void roleNames_containsExpectedKeys()
    {
        CharListModel model;
        const auto names = model.roleNames();
        QVERIFY(names.contains(CharListModel::NameRole));
        QVERIFY(names.contains(CharListModel::TakenRole));
        QVERIFY(names.contains(CharListModel::IconSourceRole));
    }

    void appendBatch_preservesOrder()
    {
        CharListModel model;
        model.appendBatch({make("A"), make("B"), make("C")});
        QCOMPARE(model.data(model.index(0), CharListModel::NameRole).toString(), QStringLiteral("A"));
        QCOMPARE(model.data(model.index(1), CharListModel::NameRole).toString(), QStringLiteral("B"));
        QCOMPARE(model.data(model.index(2), CharListModel::NameRole).toString(), QStringLiteral("C"));
    }
};

QTEST_MAIN(TestCharListModel)
#include "tst_char_list_model.moc"
