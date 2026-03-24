// Separate miniaudio compilation unit for device I/O on Apple platforms.
// Must be .mm (Objective-C++) because miniaudio's CoreAudio backend
// includes Foundation.h which requires Objective-C compilation.
//
// The decoder-only miniaudio impl lives in engine/asset/miniaudio_impl.cpp
// with MA_NO_DEVICE_IO. This file provides ONLY the device I/O symbols.
// We use MA_NO_DECODING to avoid duplicate decoder symbols.
#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MA_NO_GENERATION
#define MA_NO_ENGINE
#define MA_NO_NODE_GRAPH
#define MA_NO_RESOURCE_MANAGER
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
