#include <QTest>
#include <QSignalSpy>

#include "PlayerListModel.h"

class TestPlayerListModel : public QObject {
    Q_OBJECT

  private:
    static PlayerListModel::PlayerEntry make(int id, const char* name, const char* ch, int area)
    {
        return {id, QString(name), QString(ch), area};
    }

  private slots:
    void initial_rowCount_isZero()
    {
        PlayerListModel model;
        QCOMPARE(model.rowCount(), 0);
    }

    void reset_withEmptyMap_hasZeroRows()
    {
        PlayerListModel model;
        model.reset({});
        QCOMPARE(model.rowCount(), 0);
    }

    void reset_populatesRows()
    {
        PlayerListModel model;
        std::map<int, PlayerListModel::PlayerEntry> players = {
            {1, make(1, "Phoenix", "Phoenix_Wright", 0)},
            {2, make(2, "Edgeworth", "Miles_Edgeworth", 0)},
        };
        model.reset(players);
        QCOMPARE(model.rowCount(), 2);
    }

    void reset_replacesExistingData()
    {
        PlayerListModel model;
        model.reset({{1, make(1, "Phoenix", "pw", 0)}});
        model.reset({{5, make(5, "Maya", "maya", 1)}});
        QCOMPARE(model.rowCount(), 1);
    }

    void reset_emitsModelReset()
    {
        PlayerListModel model;
        QSignalSpy spy(&model, &QAbstractListModel::modelReset);
        model.reset({{1, make(1, "Phoenix", "pw", 0)}});
        QCOMPARE(spy.count(), 1);
    }

    void clear_resetsToEmpty()
    {
        PlayerListModel model;
        model.reset({{1, make(1, "Phoenix", "pw", 0)}});
        model.clear();
        QCOMPARE(model.rowCount(), 0);
    }

    void clear_emitsModelReset()
    {
        PlayerListModel model;
        model.reset({{1, make(1, "Phoenix", "pw", 0)}});
        QSignalSpy spy(&model, &QAbstractListModel::modelReset);
        model.clear();
        QCOMPARE(spy.count(), 1);
    }

    void data_playerIdRole()
    {
        PlayerListModel model;
        model.reset({{7, make(7, "Phoenix", "pw", 0)}});
        bool found = false;
        for (int r = 0; r < model.rowCount(); ++r) {
            if (model.data(model.index(r), PlayerListModel::PlayerIdRole).toInt() == 7) {
                found = true;
                break;
            }
        }
        QVERIFY(found);
    }

    void data_nameRole()
    {
        PlayerListModel model;
        model.reset({{1, make(1, "Phoenix Wright", "pw", 0)}});
        QCOMPARE(model.data(model.index(0), PlayerListModel::NameRole).toString(),
                 QStringLiteral("Phoenix Wright"));
    }

    void data_characterRole()
    {
        PlayerListModel model;
        model.reset({{1, make(1, "name", "Phoenix_Wright", 0)}});
        QCOMPARE(model.data(model.index(0), PlayerListModel::CharacterRole).toString(),
                 QStringLiteral("Phoenix_Wright"));
    }

    void data_areaIdRole()
    {
        PlayerListModel model;
        model.reset({{1, make(1, "name", "char", 3)}});
        QCOMPARE(model.data(model.index(0), PlayerListModel::AreaIdRole).toInt(), 3);
    }

    void data_invalidIndex_returnsInvalid()
    {
        PlayerListModel model;
        QVERIFY(!model.data(model.index(0)).isValid());
    }

    void roleNames_containsExpectedKeys()
    {
        PlayerListModel model;
        const auto names = model.roleNames();
        QVERIFY(names.contains(PlayerListModel::PlayerIdRole));
        QVERIFY(names.contains(PlayerListModel::NameRole));
        QVERIFY(names.contains(PlayerListModel::CharacterRole));
        QVERIFY(names.contains(PlayerListModel::AreaIdRole));
    }
};

QTEST_MAIN(TestPlayerListModel)
#include "tst_player_list_model.moc"
