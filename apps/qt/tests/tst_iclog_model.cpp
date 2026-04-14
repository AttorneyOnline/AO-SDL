#include <QTest>
#include <QSignalSpy>

#include "ICLogModel.h"

class TestICLogModel : public QObject {
    Q_OBJECT

  private slots:
    void initial_rowCount_isZero()
    {
        ICLogModel model;
        QCOMPARE(model.rowCount(), 0);
    }

    void appendEntry_increasesCount()
    {
        ICLogModel model;
        model.appendEntry({QStringLiteral("Phoenix"), QStringLiteral("Objection!"), 2});
        QCOMPARE(model.rowCount(), 1);
    }

    void appendEntry_emitsRowsInserted()
    {
        ICLogModel model;
        QSignalSpy spy(&model, &QAbstractListModel::rowsInserted);
        model.appendEntry({QStringLiteral("Phoenix"), QStringLiteral("Objection!"), 2});
        QCOMPARE(spy.count(), 1);
    }

    void clear_resetsToEmpty()
    {
        ICLogModel model;
        model.appendEntry({QStringLiteral("Phoenix"), QStringLiteral("Objection!"), 2});
        model.clear();
        QCOMPARE(model.rowCount(), 0);
    }

    void clear_emitsModelReset()
    {
        ICLogModel model;
        model.appendEntry({QStringLiteral("show"), QStringLiteral("msg"), 0});
        QSignalSpy spy(&model, &QAbstractListModel::modelReset);
        model.clear();
        QCOMPARE(spy.count(), 1);
    }

    // kMaxEntries is 200; inserting 250 must not exceed that.
    void maxEntries_cappedAt200()
    {
        ICLogModel model;
        for (int i = 0; i < 250; ++i)
            model.appendEntry({QString(), QStringLiteral("msg"), 0});
        QCOMPARE(model.rowCount(), 200);
    }

    void maxEntries_exactlyAtCap_doesNotGrow()
    {
        ICLogModel model;
        for (int i = 0; i < 200; ++i)
            model.appendEntry({QString(), QStringLiteral("msg"), 0});
        QCOMPARE(model.rowCount(), 200);
        model.appendEntry({QString(), QStringLiteral("one more"), 0});
        QCOMPARE(model.rowCount(), 200);
    }

    void data_shownameRole()
    {
        ICLogModel model;
        model.appendEntry({QStringLiteral("Edgeworth"), QStringLiteral("Sustained."), 1});
        QCOMPARE(model.data(model.index(0), ICLogModel::ShownameRole).toString(),
                 QStringLiteral("Edgeworth"));
    }

    void data_messageRole()
    {
        ICLogModel model;
        model.appendEntry({QStringLiteral("show"), QStringLiteral("Hold it!"), 3});
        QCOMPARE(model.data(model.index(0), ICLogModel::MessageRole).toString(),
                 QStringLiteral("Hold it!"));
    }

    void data_colorIdxRole()
    {
        ICLogModel model;
        model.appendEntry({QStringLiteral("show"), QStringLiteral("msg"), 5});
        QCOMPARE(model.data(model.index(0), ICLogModel::ColorIdxRole).toInt(), 5);
    }

    void data_colorIdxRole_defaultZero()
    {
        ICLogModel model;
        ICLogModel::Entry e;
        e.showname = QStringLiteral("show");
        e.message  = QStringLiteral("msg");
        // colorIdx defaults to 0
        model.appendEntry(e);
        QCOMPARE(model.data(model.index(0), ICLogModel::ColorIdxRole).toInt(), 0);
    }

    void data_invalidIndex_returnsInvalid()
    {
        ICLogModel model;
        QVERIFY(!model.data(model.index(0)).isValid());
    }

    void roleNames_containsExpectedKeys()
    {
        ICLogModel model;
        const auto names = model.roleNames();
        QVERIFY(names.contains(ICLogModel::ShownameRole));
        QVERIFY(names.contains(ICLogModel::MessageRole));
        QVERIFY(names.contains(ICLogModel::ColorIdxRole));
    }

    void clear_thenAppend_works()
    {
        ICLogModel model;
        model.appendEntry({QStringLiteral("A"), QStringLiteral("msg"), 0});
        model.clear();
        model.appendEntry({QStringLiteral("B"), QStringLiteral("new"), 1});
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.data(model.index(0), ICLogModel::ShownameRole).toString(),
                 QStringLiteral("B"));
    }
};

QTEST_MAIN(TestICLogModel)
#include "tst_iclog_model.moc"
