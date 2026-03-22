#include <metal_stdlib>
using namespace metal;
struct FragmentUniforms { int frame_index; float opacity; };
fragment float4 fragment_main(VertexOut in [[stage_in]],
                              texture2d_array<float> tex [[texture(0)]],
                              sampler samp [[sampler(0)]],
                              constant FragmentUniforms& u [[buffer(0)]]) {
    float4 color = tex.sample(samp, in.texcoord, u.frame_index);
    color.a *= u.opacity;
    if (color.a < 0.001) discard_fragment();
    return color;
}
