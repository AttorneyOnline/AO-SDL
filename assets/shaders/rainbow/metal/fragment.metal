// Fragment shader for rainbow effect (Metal)
// NOTE: This file is concatenated with vertex.metal by the engine.
// Only the fragment function and its types go here.

struct RainbowFragUniforms {
    int frame_index;
    float opacity;
    float u_time;
};

float3 hsv2rgb_fn(float h, float s, float v) {
    float3 c = float3(h, s, v);
    float3 rgb = clamp(abs(fmod(c.x * 6.0 + float3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    return c.z * mix(float3(1.0), rgb, c.y);
}

fragment float4 fragment_main(VertexOut in [[stage_in]],
                              texture2d_array<float> tex [[texture(0)]],
                              sampler samp [[sampler(0)]],
                              constant RainbowFragUniforms& u [[buffer(0)]]) {
    float4 color = tex.sample(samp, in.texcoord, u.frame_index);
    color.a *= u.opacity;
    if (color.a < 0.001) discard_fragment();

    float hue = fract((in.texcoord.x + in.texcoord.y) * 0.5 + u.u_time * 0.3);
    float3 rainbow = hsv2rgb_fn(hue, 0.6, 1.0);

    color.rgb = mix(color.rgb, rainbow, 0.4);

    return color;
}
