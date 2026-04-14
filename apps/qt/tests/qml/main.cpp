#include <QQmlEngine>
#include <QtPlugin>
#include <QtQuickTest/quicktest.h>

// Register the static AO.Components QML plugin so that
// "import AO.Components" resolves inside the test engine.
Q_IMPORT_QML_PLUGIN(ao_componentsplugin)

// Setup runs before any test QML file is loaded.
// It adds the build-tree QML import path so the engine can locate
// the AO.Components module's qmldir / type registrations.
class Setup : public QObject
{
    Q_OBJECT
public slots:
    void qmlEngineAvailable(QQmlEngine *engine)
    {
        engine->addImportPath(QStringLiteral(QML_IMPORT_PATH));
    }
};

#include "main.moc"

QUICK_TEST_MAIN_WITH_SETUP(tst_qml_components, Setup)
