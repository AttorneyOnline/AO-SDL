#include <QTest>
#include <QSignalSpy>

#include "MusicAreaModel.h"

class TestMusicAreaModel : public QObject {
    Q_OBJECT

  private:
    static MusicAreaModel::Entry makeTrack(const char* name)
    {
        MusicAreaModel::Entry e;
        e.name = name;
        e.isArea = false;
        e.playerCount = 0;
        return e;
    }

    static MusicAreaModel::Entry makeArea(const char* name, int players,
                                          const char* status = "", const char* cm = "",
                                          const char* lock = "")
    {
        MusicAreaModel::Entry e;
        e.name = name;
        e.isArea = true;
        e.playerCount = players;
        e.status = status;
        e.cm = cm;
        e.lock = lock;
        return e;
    }

  private slots:
    void initial_rowCount_isZero()
    {
        MusicAreaModel model;
        QCOMPARE(model.rowCount(), 0);
    }

    void reset_populatesRows()
    {
        MusicAreaModel model;
        model.reset({makeTrack("Cornered"), makeArea("Courtroom 1", 3)});
        QCOMPARE(model.rowCount(), 2);
    }

    void reset_replacesExistingData()
    {
        MusicAreaModel model;
        model.reset({makeTrack("A"), makeTrack("B")});
        model.reset({makeTrack("X")});
        QCOMPARE(model.rowCount(), 1);
    }

    void reset_emitsModelReset()
    {
        MusicAreaModel model;
        QSignalSpy spy(&model, &QAbstractListModel::modelReset);
        model.reset({makeTrack("Cornered")});
        QCOMPARE(spy.count(), 1);
    }

    void clear_resetsToEmpty()
    {
        MusicAreaModel model;
        model.reset({makeTrack("Cornered")});
        model.clear();
        QCOMPARE(model.rowCount(), 0);
    }

    void clear_emitsModelReset()
    {
        MusicAreaModel model;
        model.reset({makeTrack("Cornered")});
        QSignalSpy spy(&model, &QAbstractListModel::modelReset);
        model.clear();
        QCOMPARE(spy.count(), 1);
    }

    void data_nameRole_track()
    {
        MusicAreaModel model;
        model.reset({makeTrack("Cornered")});
        QCOMPARE(model.data(model.index(0), MusicAreaModel::NameRole).toString(),
                 QStringLiteral("Cornered"));
    }

    void data_isAreaRole_track_isFalse()
    {
        MusicAreaModel model;
        model.reset({makeTrack("Cornered")});
        QCOMPARE(model.data(model.index(0), MusicAreaModel::IsAreaRole).toBool(), false);
    }

    void data_isAreaRole_area_isTrue()
    {
        MusicAreaModel model;
        model.reset({makeArea("Courtroom 1", 3)});
        QCOMPARE(model.data(model.index(0), MusicAreaModel::IsAreaRole).toBool(), true);
    }

    void data_playerCountRole_track_isZero()
    {
        MusicAreaModel model;
        model.reset({makeTrack("Cornered")});
        QCOMPARE(model.data(model.index(0), MusicAreaModel::PlayerCountRole).toInt(), 0);
    }

    void data_playerCountRole_area()
    {
        MusicAreaModel model;
        model.reset({makeArea("Courtroom 1", 7)});
        QCOMPARE(model.data(model.index(0), MusicAreaModel::PlayerCountRole).toInt(), 7);
    }

    void data_statusRole()
    {
        MusicAreaModel model;
        model.reset({makeArea("Room", 1, "idle")});
        QCOMPARE(model.data(model.index(0), MusicAreaModel::StatusRole).toString(),
                 QStringLiteral("idle"));
    }

    void data_cmRole()
    {
        MusicAreaModel model;
        model.reset({makeArea("Room", 1, "", "Phoenix")});
        QCOMPARE(model.data(model.index(0), MusicAreaModel::CmRole).toString(),
                 QStringLiteral("Phoenix"));
    }

    void data_lockRole()
    {
        MusicAreaModel model;
        model.reset({makeArea("Room", 1, "", "", "locked")});
        QCOMPARE(model.data(model.index(0), MusicAreaModel::LockRole).toString(),
                 QStringLiteral("locked"));
    }

    void data_invalidIndex_returnsInvalid()
    {
        MusicAreaModel model;
        QVERIFY(!model.data(model.index(0)).isValid());
    }

    void roleNames_containsExpectedKeys()
    {
        MusicAreaModel model;
        const auto names = model.roleNames();
        QVERIFY(names.contains(MusicAreaModel::NameRole));
        QVERIFY(names.contains(MusicAreaModel::PlayerCountRole));
        QVERIFY(names.contains(MusicAreaModel::IsAreaRole));
        QVERIFY(names.contains(MusicAreaModel::StatusRole));
        QVERIFY(names.contains(MusicAreaModel::CmRole));
        QVERIFY(names.contains(MusicAreaModel::LockRole));
    }

    void mixedEntries_preserveOrder()
    {
        MusicAreaModel model;
        model.reset({makeTrack("Cornered"), makeArea("Room A", 2), makeTrack("Pursuit")});
        QCOMPARE(model.data(model.index(0), MusicAreaModel::IsAreaRole).toBool(), false);
        QCOMPARE(model.data(model.index(1), MusicAreaModel::IsAreaRole).toBool(), true);
        QCOMPARE(model.data(model.index(2), MusicAreaModel::IsAreaRole).toBool(), false);
    }
};

QTEST_MAIN(TestMusicAreaModel)
#include "tst_music_area_model.moc"
