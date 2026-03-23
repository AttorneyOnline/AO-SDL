Pod::Spec.new do |s|
  s.name         = 'ao_bridge'
  s.version      = '1.0.0'
  s.summary      = 'AO-SDL native bridge for Flutter'
  s.homepage     = 'https://aceattorneyonline.com'
  s.license      = { :type => 'MIT' }
  s.author       = 'AO'
  s.platform     = :ios, '17.0'
  s.source       = { :path => '.' }

  # Pre-built static libraries from CMake (symlinked via libs/ios-sim)
  s.vendored_libraries =
    'libs/ios-sim/libao_bridge.a',
    'libs/ios-sim/libao_protocol.a',
    'libs/ios-sim/libao_game.a',
    'libs/ios-sim/libao_net.a',
    'libs/ios-sim/libaoengine.a',
    'libs/ios-sim/libaorender_metal.a',
    'libs/ios-sim/libopusfile.a',
    'libs/ios-sim/lib/libbit7z64_d.a',
    'libs/ios-sim/third-party/freetype/libfreetyped.a',
    'libs/ios-sim/third-party/libwebp/libwebp.a',
    'libs/ios-sim/third-party/libwebp/libwebpdecoder.a',
    'libs/ios-sim/third-party/libwebp/libwebpdemux.a',
    'libs/ios-sim/third-party/libwebp/libsharpyuv.a',
    'libs/ios-sim/third-party/ogg/libogg.a',
    'libs/ios-sim/third-party/opus/libopus.a'

  s.frameworks = 'Metal', 'MetalKit', 'QuartzCore', 'CoreText', 'Foundation', 'IOKit'
  s.libraries = 'c++', 'z'

  s.pod_target_xcconfig = {
    'CLANG_CXX_LANGUAGE_STANDARD' => 'c++20',
  }
  # Force the Runner to link all symbols from our static libs,
  # not just those referenced at compile time. Dart FFI resolves
  # symbols at runtime via dlsym, so the linker would otherwise
  # strip them as "unused".
  s.user_target_xcconfig = {
    'OTHER_LDFLAGS' => '-ObjC -all_load',
  }
end
