#include <QQmlEngine>
#include <QtQuickTest/quicktest.h>

// The AO.Components static QML plugin is registered by linking the
// Qt-generated `ao_componentsplugin_init` object library in
// CMakeLists.txt — that object contains the Q_IMPORT_PLUGIN expansion
// and force-loads qt_static_plugin_ao_componentsplugin() into the
// binary, so "import AO.Components" resolves inside the test engine.

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
