#include <metal_stdlib>
using namespace metal;
struct VertexOut { float4 position [[position]]; float2 texcoord; };
vertex VertexOut blit_vertex(uint vid [[vertex_id]]) {
    float2 pos[4] = { {-1,-1}, {1,-1}, {-1,1}, {1,1} };
    float2 uv[4]  = { {0,1}, {1,1}, {0,0}, {1,0} };
    VertexOut out;
    out.position = float4(pos[vid], 0, 1);
    out.texcoord = uv[vid];
    return out;
}
