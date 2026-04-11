#include "IQtRenderBackend.h"

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>

/// OpenGL backend. GLRenderer operates inside Qt's shared OpenGL context,
/// so we must verify context existence before init and rebind the default FBO
/// after each draw so Qt's RHI can composite the scene graph on top.
class GLRenderBackend : public IQtRenderBackend {
  public:
    bool isContextReady() const override {
        return QOpenGLContext::currentContext() != nullptr;
    }

    void restoreState() const override {
        // GLRenderer leaves its own FBO bound; rebind default so Qt's RHI can render on top.
        QOpenGLContext::currentContext()->extraFunctions()->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    QRhiTexture::Format textureFormat() const override { return QRhiTexture::RGBA8; }
    const char* backendName() const override { return "OpenGL"; }
};

std::unique_ptr<IQtRenderBackend> create_qt_render_backend() {
    return std::make_unique<GLRenderBackend>();
}
