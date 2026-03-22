#include <metal_stdlib>
using namespace metal;
struct WFIn { float2 position [[attribute(0)]]; float2 texcoord [[attribute(1)]]; };
struct WFOut { float4 position [[position]]; };
struct WFUniforms { float4x4 local; float aspect; };
vertex WFOut wf_vertex(WFIn in [[stage_in]], constant WFUniforms& u [[buffer(1)]]) {
    WFOut out;
    out.position = u.local * float4(in.position, 0.0, 1.0);
    return out;
}
