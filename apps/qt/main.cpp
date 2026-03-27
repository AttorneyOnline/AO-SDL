#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>

int main(int argc, char* argv[]) {
#if !defined(Q_OS_APPLE)
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
#endif

    QGuiApplication app(argc, argv);
    app.setApplicationName("Attorney Online");

    QQmlApplicationEngine engine;
    engine.loadFromModule("AO", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
