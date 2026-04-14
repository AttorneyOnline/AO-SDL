#include <QTest>
#include <QSignalSpy>

#include "EmoteModel.h"

class TestEmoteModel : public QObject {
    Q_OBJECT

  private slots:
    void initial_rowCount_isZero()
    {
        EmoteModel model;
        QCOMPARE(model.rowCount(), 0);
    }

    void reset_populatesRows()
    {
        EmoteModel model;
        model.reset({{QStringLiteral("happy"), QString()},
                     {QStringLiteral("angry"), QString()}});
        QCOMPARE(model.rowCount(), 2);
    }

    void reset_replacesExistingData()
    {
        EmoteModel model;
        model.reset({{QStringLiteral("a"), QString()}, {QStringLiteral("b"), QString()}});
        model.reset({{QStringLiteral("x"), QString()}});
        QCOMPARE(model.rowCount(), 1);
    }

    void reset_emitsModelReset()
    {
        EmoteModel model;
        QSignalSpy spy(&model, &QAbstractListModel::modelReset);
        model.reset({{QStringLiteral("happy"), QString()}});
        QCOMPARE(spy.count(), 1);
    }

    void clear_resetsToEmpty()
    {
        EmoteModel model;
        model.reset({{QStringLiteral("happy"), QString()}});
        model.clear();
        QCOMPARE(model.rowCount(), 0);
    }

    void clear_emitsModelReset()
    {
        EmoteModel model;
        model.reset({{QStringLiteral("happy"), QString()}});
        QSignalSpy spy(&model, &QAbstractListModel::modelReset);
        model.clear();
        QCOMPARE(spy.count(), 1);
    }

    void data_commentRole()
    {
        EmoteModel model;
        model.reset({{QStringLiteral("happy"), QString()}});
        QCOMPARE(model.data(model.index(0), EmoteModel::CommentRole).toString(),
                 QStringLiteral("happy"));
    }

    void data_iconSourceRole_empty()
    {
        EmoteModel model;
        model.reset({{QStringLiteral("happy"), QString()}});
        QCOMPARE(model.data(model.index(0), EmoteModel::IconSourceRole).toString(), QString());
    }

    void data_iconSourceRole_set()
    {
        EmoteModel model;
        model.reset({{QStringLiteral("happy"), QStringLiteral("image://emoteicon/Phoenix/0")}});
        QCOMPARE(model.data(model.index(0), EmoteModel::IconSourceRole).toString(),
                 QStringLiteral("image://emoteicon/Phoenix/0"));
    }

    void data_invalidIndex_returnsInvalid()
    {
        EmoteModel model;
        QVERIFY(!model.data(model.index(0)).isValid());
    }

    void roleNames_containsExpectedKeys()
    {
        EmoteModel model;
        const auto names = model.roleNames();
        QVERIFY(names.contains(EmoteModel::CommentRole));
        QVERIFY(names.contains(EmoteModel::IconSourceRole));
    }

    void reset_preservesOrder()
    {
        EmoteModel model;
        model.reset({{QStringLiteral("A"), QString()},
                     {QStringLiteral("B"), QString()},
                     {QStringLiteral("C"), QString()}});
        QCOMPARE(model.data(model.index(0), EmoteModel::CommentRole).toString(), QStringLiteral("A"));
        QCOMPARE(model.data(model.index(1), EmoteModel::CommentRole).toString(), QStringLiteral("B"));
        QCOMPARE(model.data(model.index(2), EmoteModel::CommentRole).toString(), QStringLiteral("C"));
    }

    void clear_thenReset_works()
    {
        EmoteModel model;
        model.reset({{QStringLiteral("a"), QString()}});
        model.clear();
        model.reset({{QStringLiteral("b"), QString()}});
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.data(model.index(0), EmoteModel::CommentRole).toString(),
                 QStringLiteral("b"));
    }
};

QTEST_MAIN(TestEmoteModel)
#include "tst_emote_model.moc"
