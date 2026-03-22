#include <QDebug>
#include <QGuiApplication>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QQuickWindow>
#include <QSGRendererInterface>

#include "renderer/IGPUBackend.h"

static void configure_process() {
    // Flush stdout immediately (workaround for Qt Creator's buffered output)
    std::setvbuf(stdout, nullptr, _IONBF, 0);

#ifndef _WIN32
    // Return EPIPE on writes to a closed socket rather than terminating the
    // process. The socket layer will surface this as an exception instead.
    std::signal(SIGPIPE, SIG_IGN);
#endif
}

int main(int argc, char* argv[]) {
    configure_process();
    IGPUBackend::pre_init();

    QGuiApplication app(argc, argv);

    return app.exec();
}
