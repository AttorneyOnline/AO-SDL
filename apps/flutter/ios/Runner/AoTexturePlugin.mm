#import "AoTexturePlugin.h"
#import <CoreVideo/CoreVideo.h>
#import <Metal/Metal.h>

// C bridge functions — defined in libao_bridge.a
extern "C" {
void ao_renderer_create(int width, int height, bool use_metal);
void ao_renderer_draw(void);
uintptr_t ao_renderer_get_texture(void);
void ao_renderer_resize(int width, int height);
void *ao_renderer_get_metal_device(void);
void *ao_renderer_get_metal_command_queue(void);
}

// Wraps the Metal render texture as a CVPixelBuffer for Flutter's Texture widget.
@interface AoFlutterTexture : NSObject <FlutterTexture>
@property(nonatomic, assign) int width;
@property(nonatomic, assign) int height;
@property(nonatomic, assign) bool rendererReady;
@end

@implementation AoFlutterTexture {
    CVPixelBufferRef _pixelBuffer;
    CVMetalTextureCacheRef _textureCache;
    id<MTLDevice> _device;
    id<MTLCommandQueue> _commandQueue;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _width = 0;
        _height = 0;
        _rendererReady = false;
        _pixelBuffer = NULL;
        _textureCache = NULL;
    }
    return self;
}

- (void)setupWithWidth:(int)width height:(int)height {
    _width = width;
    _height = height;

    // Create the renderer — this sets up the Metal device and offscreen FBO
    ao_renderer_create(width, height, true);
    _rendererReady = true;

    _device = (__bridge id<MTLDevice>)ao_renderer_get_metal_device();
    _commandQueue = (__bridge id<MTLCommandQueue>)ao_renderer_get_metal_command_queue();

    if (_device) {
        CVMetalTextureCacheCreate(kCFAllocatorDefault, NULL, _device, NULL, &_textureCache);
    }

    [self _ensurePixelBuffer];
}

- (void)_ensurePixelBuffer {
    if (_pixelBuffer && CVPixelBufferGetWidth(_pixelBuffer) == _width &&
        CVPixelBufferGetHeight(_pixelBuffer) == _height) {
        return;
    }

    if (_pixelBuffer) {
        CVPixelBufferRelease(_pixelBuffer);
        _pixelBuffer = NULL;
    }

    NSDictionary *attrs = @{
        (__bridge NSString *)kCVPixelBufferMetalCompatibilityKey : @YES,
        (__bridge NSString *)kCVPixelBufferIOSurfacePropertiesKey : @{},
    };

    CVPixelBufferCreate(kCFAllocatorDefault, _width, _height, kCVPixelFormatType_32BGRA,
                        (__bridge CFDictionaryRef)attrs, &_pixelBuffer);
}

- (CVPixelBufferRef)copyPixelBuffer {
    if (!_rendererReady || !_device || !_commandQueue || !_textureCache)
        return NULL;

    // Draw the courtroom scene to the offscreen Metal texture
    ao_renderer_draw();

    uintptr_t texId = ao_renderer_get_texture();
    if (texId == 0)
        return NULL;

    id<MTLTexture> renderTex = (__bridge id<MTLTexture>)(void *)texId;

    [self _ensurePixelBuffer];
    if (!_pixelBuffer)
        return NULL;

    // Wrap the CVPixelBuffer as a Metal texture so we can blit into it
    CVMetalTextureRef cvTex = NULL;
    CVMetalTextureCacheCreateTextureFromImage(kCFAllocatorDefault, _textureCache, _pixelBuffer, NULL,
                                              MTLPixelFormatBGRA8Unorm, _width, _height, 0, &cvTex);

    if (!cvTex)
        return NULL;

    id<MTLTexture> dstTex = CVMetalTextureGetTexture(cvTex);

    // Blit from render texture to the CVPixelBuffer-backed texture
    id<MTLCommandBuffer> cmdBuf = [_commandQueue commandBuffer];
    id<MTLBlitCommandEncoder> blit = [cmdBuf blitCommandEncoder];
    [blit copyFromTexture:renderTex
              sourceSlice:0
              sourceLevel:0
             sourceOrigin:MTLOriginMake(0, 0, 0)
               sourceSize:MTLSizeMake(_width, _height, 1)
                toTexture:dstTex
         destinationSlice:0
         destinationLevel:0
        destinationOrigin:MTLOriginMake(0, 0, 0)];
    [blit endEncoding];
    [cmdBuf commit];
    [cmdBuf waitUntilCompleted];

    CFRelease(cvTex);

    CVPixelBufferRetain(_pixelBuffer);
    return _pixelBuffer;
}

- (void)dealloc {
    if (_pixelBuffer)
        CVPixelBufferRelease(_pixelBuffer);
    if (_textureCache)
        CFRelease(_textureCache);
}

@end

// ---------------------------------------------------------------------------
// Plugin registration
// ---------------------------------------------------------------------------

@implementation AoTexturePlugin {
    NSObject<FlutterTextureRegistry> *_registry;
    AoFlutterTexture *_texture;
    int64_t _textureId;
    CADisplayLink *_displayLink;
}

+ (void)registerWithRegistrar:(NSObject<FlutterPluginRegistrar> *)registrar {
    FlutterMethodChannel *channel = [FlutterMethodChannel methodChannelWithName:@"ao_texture"
                                                                binaryMessenger:[registrar messenger]];
    AoTexturePlugin *instance = [[AoTexturePlugin alloc] initWithRegistrar:registrar];
    [registrar addMethodCallDelegate:instance channel:channel];
}

- (instancetype)initWithRegistrar:(NSObject<FlutterPluginRegistrar> *)registrar {
    self = [super init];
    if (self) {
        _registry = [registrar textures];
        _texture = [[AoFlutterTexture alloc] init];
        _textureId = -1;
    }
    return self;
}

- (void)handleMethodCall:(FlutterMethodCall *)call result:(FlutterResult)result {
    if ([@"create" isEqualToString:call.method]) {
        int width = [call.arguments[@"width"] intValue];
        int height = [call.arguments[@"height"] intValue];

        [_texture setupWithWidth:width height:height];
        _textureId = [_registry registerTexture:_texture];

        // Start a display link to notify Flutter of new frames
        _displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(_onFrame)];
        [_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];

        result(@(_textureId));
    } else if ([@"dispose" isEqualToString:call.method]) {
        [_displayLink invalidate];
        _displayLink = nil;
        if (_textureId >= 0) {
            [_registry unregisterTexture:_textureId];
            _textureId = -1;
        }
        result(nil);
    } else {
        result(FlutterMethodNotImplemented);
    }
}

- (void)_onFrame {
    if (_textureId >= 0) {
        [_registry textureFrameAvailable:_textureId];
    }
}

@end
