#include <QTest>
#include <QSignalSpy>

#include "ChatModel.h"

class TestChatModel : public QObject {
    Q_OBJECT

  private slots:
    void initial_rowCount_isZero()
    {
        ChatModel model;
        QCOMPARE(model.rowCount(), 0);
    }

    void appendLine_increasesCount()
    {
        ChatModel model;
        model.appendLine({QStringLiteral("Alice"), QStringLiteral("Hello!"), false});
        QCOMPARE(model.rowCount(), 1);
    }

    void appendLine_emitsRowsInserted()
    {
        ChatModel model;
        QSignalSpy spy(&model, &QAbstractListModel::rowsInserted);
        model.appendLine({QStringLiteral("Alice"), QStringLiteral("Hello!"), false});
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(1).toInt(), 0); // first row
        QCOMPARE(spy.at(0).at(2).toInt(), 0); // last row (single entry)
    }

    void clear_resetsToEmpty()
    {
        ChatModel model;
        model.appendLine({QStringLiteral("Alice"), QStringLiteral("Hello!"), false});
        model.clear();
        QCOMPARE(model.rowCount(), 0);
    }

    void clear_emitsModelReset()
    {
        ChatModel model;
        model.appendLine({QStringLiteral("Alice"), QStringLiteral("msg"), false});
        QSignalSpy spy(&model, &QAbstractListModel::modelReset);
        model.clear();
        QCOMPARE(spy.count(), 1);
    }

    void data_senderRole()
    {
        ChatModel model;
        model.appendLine({QStringLiteral("Alice"), QStringLiteral("msg"), false});
        QCOMPARE(model.data(model.index(0), ChatModel::SenderRole).toString(),
                 QStringLiteral("Alice"));
    }

    void data_messageRole()
    {
        ChatModel model;
        model.appendLine({QStringLiteral("sender"), QStringLiteral("Hello World"), false});
        QCOMPARE(model.data(model.index(0), ChatModel::MessageRole).toString(),
                 QStringLiteral("Hello World"));
    }

    void data_isSystemRole_false()
    {
        ChatModel model;
        model.appendLine({QStringLiteral("Alice"), QStringLiteral("msg"), false});
        QCOMPARE(model.data(model.index(0), ChatModel::IsSystemRole).toBool(), false);
    }

    void data_isSystemRole_true()
    {
        ChatModel model;
        model.appendLine({QString(), QStringLiteral("Server started."), true});
        QCOMPARE(model.data(model.index(0), ChatModel::IsSystemRole).toBool(), true);
    }

    void data_invalidIndex_returnsInvalid()
    {
        ChatModel model;
        QVERIFY(!model.data(model.index(0)).isValid());
    }

    void roleNames_containsExpectedKeys()
    {
        ChatModel model;
        const auto names = model.roleNames();
        QVERIFY(names.contains(ChatModel::SenderRole));
        QVERIFY(names.contains(ChatModel::MessageRole));
        QVERIFY(names.contains(ChatModel::IsSystemRole));
    }

    void multipleLines_preserveInsertionOrder()
    {
        ChatModel model;
        model.appendLine({QStringLiteral("A"), QStringLiteral("first"), false});
        model.appendLine({QStringLiteral("B"), QStringLiteral("second"), false});
        QCOMPARE(model.data(model.index(0), ChatModel::SenderRole).toString(), QStringLiteral("A"));
        QCOMPARE(model.data(model.index(1), ChatModel::SenderRole).toString(), QStringLiteral("B"));
    }

    void clear_thenAppend_works()
    {
        ChatModel model;
        model.appendLine({QStringLiteral("A"), QStringLiteral("msg"), false});
        model.clear();
        model.appendLine({QStringLiteral("B"), QStringLiteral("new"), false});
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.data(model.index(0), ChatModel::SenderRole).toString(), QStringLiteral("B"));
    }
};

QTEST_MAIN(TestChatModel)
#include "tst_chat_model.moc"
