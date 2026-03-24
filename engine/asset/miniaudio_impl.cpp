// Single compilation unit for miniaudio (mirrors stb_image_impl.cpp).
// Only the decoder is needed — device I/O is handled by the platform backend.
#define MA_NO_DEVICE_IO
#define MA_NO_GENERATION
#define MA_NO_ENGINE
#define MA_NO_NODE_GRAPH
#define MA_NO_RESOURCE_MANAGER
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
