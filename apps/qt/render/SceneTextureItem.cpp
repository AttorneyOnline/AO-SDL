#include "SceneTextureItem.h"

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSGSimpleTextureNode>
#include <rhi/qrhi.h>

#include <cmath>

// --------------------------------------------------------------------------
// Shaders (GLSL 330 core)
// --------------------------------------------------------------------------

static const char* kVertexShader = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec3 aCol;
out vec3 vCol;
uniform float uAngle;
void main() {
    float c = cos(uAngle);
    float s = sin(uAngle);
    vec2 p  = vec2(aPos.x * c - aPos.y * s,
                   aPos.x * s + aPos.y * c);
    gl_Position = vec4(p, 0.0, 1.0);
    vCol = aCol;
}
)";

static const char* kFragmentShader = R"(
#version 330 core
in  vec3 vCol;
out vec4 fragColor;
void main() {
    fragColor = vec4(vCol, 1.0);
}
)";

// --------------------------------------------------------------------------
// Construction / destruction
// --------------------------------------------------------------------------

SceneTextureItem::SceneTextureItem(QQuickItem* parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    connect(this, &QQuickItem::windowChanged,
            this, &SceneTextureItem::handleWindowChanged);
}

SceneTextureItem::~SceneTextureItem() {
    // Normal path: cleanupGL() was already called via sceneGraphInvalidated.
    delete m_rhiTexture;
}

// --------------------------------------------------------------------------
// Window / scene-graph lifecycle
// --------------------------------------------------------------------------

void SceneTextureItem::handleWindowChanged(QQuickWindow* win) {
    if (!win)
        return;

    // DirectConnection — these fire on the render thread.
    connect(win, &QQuickWindow::beforeRendering,
            this, &SceneTextureItem::renderGL,
            Qt::DirectConnection);
    connect(win, &QQuickWindow::sceneGraphInvalidated,
            this, &SceneTextureItem::handleSceneGraphInvalidated,
            Qt::DirectConnection);
}

void SceneTextureItem::handleSceneGraphInvalidated() {
    cleanupGL();
}

// --------------------------------------------------------------------------
// GL initialisation (runs once, on the render thread)
// --------------------------------------------------------------------------

static uint compileShader(QOpenGLExtraFunctions* gl, uint type, const char* src) {
    uint s = gl->glCreateShader(type);
    gl->glShaderSource(s, 1, &src, nullptr);
    gl->glCompileShader(s);
    int ok = 0;
    gl->glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        gl->glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        qWarning("SceneTextureItem shader error: %s", log);
    }
    return s;
}

void SceneTextureItem::initGL() {
    auto* ctx = QOpenGLContext::currentContext();
    if (!ctx)
        return;
    m_gl = ctx->extraFunctions();

    m_texW = 512;
    m_texH = 512;

    // ── FBO ──────────────────────────────────────────────────────────────
    m_gl->glGenTextures(1, &m_fboTexture);
    m_gl->glBindTexture(GL_TEXTURE_2D, m_fboTexture);
    m_gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                       m_texW, m_texH, 0,
                       GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    m_gl->glBindTexture(GL_TEXTURE_2D, 0);

    m_gl->glGenFramebuffers(1, &m_fbo);
    m_gl->glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    m_gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                 GL_TEXTURE_2D, m_fboTexture, 0);
    if (m_gl->glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        qWarning("SceneTextureItem: FBO incomplete");
    m_gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ── Shader program ───────────────────────────────────────────────────
    uint vs = compileShader(m_gl, GL_VERTEX_SHADER,   kVertexShader);
    uint fs = compileShader(m_gl, GL_FRAGMENT_SHADER, kFragmentShader);

    m_program = m_gl->glCreateProgram();
    m_gl->glAttachShader(m_program, vs);
    m_gl->glAttachShader(m_program, fs);
    m_gl->glLinkProgram(m_program);

    int ok = 0;
    m_gl->glGetProgramiv(m_program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        m_gl->glGetProgramInfoLog(m_program, sizeof(log), nullptr, log);
        qWarning("SceneTextureItem link error: %s", log);
    }
    m_gl->glDeleteShader(vs);
    m_gl->glDeleteShader(fs);

    // ── Triangle geometry (pos.xy + col.rgb) ─────────────────────────────
    // clang-format off
    static const float verts[] = {
        //  x      y       r     g     b
         0.0f,  0.6f,   1.0f, 0.0f, 0.0f,   // top      — red
        -0.6f, -0.4f,   0.0f, 1.0f, 0.0f,   // left     — green
         0.6f, -0.4f,   0.0f, 0.0f, 1.0f,   // right    — blue
    };
    // clang-format on

    m_gl->glGenVertexArrays(1, &m_vao);
    m_gl->glGenBuffers(1, &m_vbo);

    m_gl->glBindVertexArray(m_vao);
    m_gl->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    m_gl->glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    m_gl->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                                5 * sizeof(float), nullptr);
    m_gl->glEnableVertexAttribArray(0);
    m_gl->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                                5 * sizeof(float),
                                reinterpret_cast<void*>(2 * sizeof(float)));
    m_gl->glEnableVertexAttribArray(1);

    m_gl->glBindVertexArray(0);
    m_gl->glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_glInitialized = true;
}

// --------------------------------------------------------------------------
// Per-frame GL rendering (render thread, via beforeRendering)
// --------------------------------------------------------------------------

void SceneTextureItem::renderGL() {
    if (!m_glInitialized)
        initGL();
    if (!m_glInitialized)
        return;

    window()->beginExternalCommands();

    m_gl->glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    m_gl->glViewport(0, 0, m_texW, m_texH);

    m_gl->glClearColor(0.10f, 0.10f, 0.15f, 1.0f);
    m_gl->glClear(GL_COLOR_BUFFER_BIT);

    m_gl->glUseProgram(m_program);
    m_gl->glUniform1f(m_gl->glGetUniformLocation(m_program, "uAngle"),
                      m_angle);
    m_angle += 0.016f;

    m_gl->glBindVertexArray(m_vao);
    m_gl->glDrawArrays(GL_TRIANGLES, 0, 3);
    m_gl->glBindVertexArray(0);

    m_gl->glUseProgram(0);
    m_gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);

    window()->endExternalCommands();
}

// --------------------------------------------------------------------------
// Scene-graph texture node (render thread, during sync)
// --------------------------------------------------------------------------

QSGNode* SceneTextureItem::updatePaintNode(QSGNode* old, UpdatePaintNodeData*) {
    if (!m_fboTexture) {
        // FBO not ready yet — will be created on the first beforeRendering.
        update();
        return old;
    }

    auto* node = static_cast<QSGSimpleTextureNode*>(old);
    if (!node) {
        node = new QSGSimpleTextureNode();
        node->setOwnsTexture(true);
    }

    // Wrap the native GL texture once (ID never changes after init).
    uintptr_t texId = static_cast<uintptr_t>(m_fboTexture);
    if (texId != m_cachedTexId) {
        auto* ri  = window()->rendererInterface();
        auto* rhi = static_cast<QRhi*>(
            ri->getResource(window(), QSGRendererInterface::RhiResource));

        if (rhi) {
            delete m_rhiTexture;
            m_rhiTexture = nullptr;

            auto* rhiTex = rhi->newTexture(
                QRhiTexture::RGBA8, QSize(m_texW, m_texH));
            if (rhiTex) {
                rhiTex->createFrom({static_cast<quint64>(texId), 0});
                m_rhiTexture  = rhiTex;
                m_cachedTexId = texId;
                node->setTexture(
                    window()->createTextureFromRhiTexture(rhiTex));
            }
        }
    }

    // OpenGL FBOs are bottom-up; flip to match QML's top-down coordinates.
    node->setTextureCoordinatesTransform(
        QSGSimpleTextureNode::MirrorVertically);
    node->setRect(boundingRect());

    // Keep the render loop alive.
    update();
    return node;
}

// --------------------------------------------------------------------------
// Cleanup (render thread)
// --------------------------------------------------------------------------

void SceneTextureItem::cleanupGL() {
    if (!m_glInitialized)
        return;

    m_gl->glDeleteVertexArrays(1, &m_vao);
    m_gl->glDeleteBuffers(1, &m_vbo);
    m_gl->glDeleteProgram(m_program);
    m_gl->glDeleteFramebuffers(1, &m_fbo);
    m_gl->glDeleteTextures(1, &m_fboTexture);

    m_vao = m_vbo = m_program = m_fbo = m_fboTexture = 0;

    delete m_rhiTexture;
    m_rhiTexture  = nullptr;
    m_cachedTexId = 0;

    m_glInitialized = false;
}
