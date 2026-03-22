#include <metal_stdlib>
using namespace metal;
fragment float4 blit_fragment(VertexOut in [[stage_in]],
                              texture2d<float> tex [[texture(0)]],
                              sampler samp [[sampler(0)]]) {
    return tex.sample(samp, in.texcoord);
}
