// Apple-only: wraps a Metal texture handle in a QSGTexture for Qt's scene graph.
// Must be compiled as Objective-C++ (.mm) because id<MTLTexture> is an ObjC type.

#import <Metal/Metal.h>

#include <QQuickWindow>
#include <QSGTexture>
#include <QtQuick/QSGMetalTexture>

#include <cstdint>

QSGTexture* tex_from_native(uintptr_t texId, QQuickWindow* window,
                             int renderW, int renderH) {
    id<MTLTexture> mtlTex = (__bridge id<MTLTexture>)reinterpret_cast<void*>(texId);
    return QNativeInterface::QSGMetalTexture::fromNative(
        mtlTex, window, QSize(renderW, renderH));
}
