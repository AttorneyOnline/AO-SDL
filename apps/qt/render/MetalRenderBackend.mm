#include "IQtRenderBackend.h"

/// Metal backend for the Qt render bridge.
///
/// MetalRenderer creates its own MTLDevice and MTLCommandQueue internally,
/// so there is no shared GPU context to guard or restore.  Qt's RHI Metal
/// backend operates independently on its own command queue.
class MetalRenderBackend : public IQtRenderBackend {
  public:
    bool isContextReady() const override { return true; }
    void restoreState() const override { /* no-op */ }
    QRhiTexture::Format textureFormat() const override {
        return QRhiTexture::BGRA8;
    }
    const char* backendName() const override { return "Metal"; }
};

std::unique_ptr<IQtRenderBackend> create_qt_render_backend() {
    return std::make_unique<MetalRenderBackend>();
}
