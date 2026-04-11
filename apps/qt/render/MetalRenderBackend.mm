#include "IQtRenderBackend.h"

/// Metal backend. MetalRenderer creates its own MTLDevice/MTLCommandQueue,
/// so there is no shared context to guard or restore.
class MetalRenderBackend : public IQtRenderBackend {
  public:
    bool isContextReady() const override { return true; }
    void restoreState() const override {}
    QRhiTexture::Format textureFormat() const override { return QRhiTexture::BGRA8; }
    const char* backendName() const override { return "Metal"; }
};

std::unique_ptr<IQtRenderBackend> create_qt_render_backend() {
    return std::make_unique<MetalRenderBackend>();
}
