#include <metal_stdlib>
using namespace metal;

struct VertexIn { float2 position [[attribute(0)]]; float2 texcoord [[attribute(1)]]; };
struct VertexOut { float4 position [[position]]; float2 texcoord; };
struct VertexUniforms { float4x4 local; float aspect; };

vertex VertexOut vertex_main(VertexIn in [[stage_in]],
                             constant VertexUniforms& u [[buffer(1)]]) {
    VertexOut out;
    out.position = u.local * float4(in.position, 0.0, 1.0);
    out.texcoord = in.texcoord;
    return out;
}
