#include <metal_stdlib>
using namespace metal;
fragment float4 wf_fragment(WFOut in [[stage_in]]) {
    return float4(0.0, 1.0, 0.0, 1.0);
}
