#import <Metal/Metal.h>

#include <QQuickGraphicsDevice>

// Called from QtGameWindow.cpp (plain C++) to avoid pulling ObjC Metal types
// into a .cpp translation unit.
QQuickGraphicsDevice qt_make_metal_graphics_device(void* device, void* queue)
{
    return QQuickGraphicsDevice::fromDeviceAndCommandQueue(
        (__bridge MTLDevice*)device,
        (__bridge MTLCommandQueue*)queue);
}
