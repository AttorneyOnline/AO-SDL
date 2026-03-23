#include "qtgamewindow.h"

#include "utils/Log.h"

#include <QQuickView>

QtGameWindow::QtGameWindow(UIManager& ui_manager, std::unique_ptr<IGPUBackend> backend)
    : window(nullptr), ui_manager(ui_manager), gpu(std::move(backend)) {

    window = new QQuickView(nullptr);
    if (window->status() == QQuickView::Status::Error) {
        QString error_string;
        for (const auto& error : window->errors()) {
            error_string.append(error.toString());
        }
        Log::log_print(LogLevel::FATAL, "Failed to create window: %s", error_string.toStdString().c_str());
    }
    gpu->create_context(window);

    window->show();
}
