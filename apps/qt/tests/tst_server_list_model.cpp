#include <QTest>
#include <QSignalSpy>

#include "ServerListModel.h"
#include "game/ServerList.h"

class TestServerListModel : public QObject {
    Q_OBJECT

  private:
    static ServerEntry make(const char* name, const char* desc, int players, const char* host)
    {
        ServerEntry e;
        e.name = name;
        e.description = desc;
        e.players = players;
        e.hostname = host;
        return e;
    }

  private slots:
    void initial_rowCount_isZero()
    {
        ServerListModel model;
        QCOMPARE(model.rowCount(), 0);
    }

    void reset_populatesRows()
    {
        ServerListModel model;
        model.reset({make("Lobby", "Fun server", 5, "ao.example.com")});
        QCOMPARE(model.rowCount(), 1);
    }

    void reset_replacesExistingData()
    {
        ServerListModel model;
        model.reset({make("A", "", 0, "a.test"), make("B", "", 0, "b.test")});
        model.reset({make("X", "", 0, "x.test")});
        QCOMPARE(model.rowCount(), 1);
    }

    void data_nameRole()
    {
        ServerListModel model;
        model.reset({make("TestServer", "desc", 3, "host")});
        QCOMPARE(model.data(model.index(0), ServerListModel::NameRole).toString(),
                 QStringLiteral("TestServer"));
    }

    void data_descriptionRole()
    {
        ServerListModel model;
        model.reset({make("name", "My Description", 3, "host")});
        QCOMPARE(model.data(model.index(0), ServerListModel::DescriptionRole).toString(),
                 QStringLiteral("My Description"));
    }

    void data_playersRole()
    {
        ServerListModel model;
        model.reset({make("name", "desc", 42, "host")});
        QCOMPARE(model.data(model.index(0), ServerListModel::PlayersRole).toInt(), 42);
    }

    void data_hostnameRole()
    {
        ServerListModel model;
        model.reset({make("name", "desc", 0, "ao.example.com")});
        QCOMPARE(model.data(model.index(0), ServerListModel::HostnameRole).toString(),
                 QStringLiteral("ao.example.com"));
    }

    void data_invalidIndex_returnsInvalid()
    {
        ServerListModel model;
        QVERIFY(!model.data(model.index(0)).isValid());
    }

    void roleNames_containsExpectedKeys()
    {
        ServerListModel model;
        const auto names = model.roleNames();
        QVERIFY(names.contains(ServerListModel::NameRole));
        QVERIFY(names.contains(ServerListModel::DescriptionRole));
        QVERIFY(names.contains(ServerListModel::PlayersRole));
        QVERIFY(names.contains(ServerListModel::HostnameRole));
    }

    void reset_emitsModelReset()
    {
        ServerListModel model;
        QSignalSpy spy(&model, &QAbstractListModel::modelReset);
        model.reset({make("A", "", 0, "a")});
        QCOMPARE(spy.count(), 1);
    }

    void reset_toEmpty_emitsModelReset()
    {
        ServerListModel model;
        model.reset({make("A", "", 0, "a")});
        QSignalSpy spy(&model, &QAbstractListModel::modelReset);
        model.reset({});
        QCOMPARE(model.rowCount(), 0);
        QCOMPARE(spy.count(), 1);
    }

    void multipleEntries_preserveOrder()
    {
        ServerListModel model;
        model.reset({make("First", "", 0, ""), make("Second", "", 0, "")});
        QCOMPARE(model.data(model.index(0), ServerListModel::NameRole).toString(),
                 QStringLiteral("First"));
        QCOMPARE(model.data(model.index(1), ServerListModel::NameRole).toString(),
                 QStringLiteral("Second"));
    }
};

QTEST_MAIN(TestServerListModel)
#include "tst_server_list_model.moc"
