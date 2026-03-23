struct RainbowTextFragUniforms {
    int frame_index;
    float opacity;
    int frame_count;
    float u_time;
};

float3 hsv2rgb_text(float h, float s, float v) {
    float3 c = float3(h, s, v);
    float3 rgb = clamp(abs(fmod(c.x * 6.0 + float3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    return c.z * mix(float3(1.0), rgb, c.y);
}

fragment float4 fragment_main(VertexOut in [[stage_in]],
                              texture2d<float> tex [[texture(0)]],
                              sampler samp [[sampler(0)]],
                              constant RainbowTextFragUniforms& u [[buffer(0)]]) {
    float inv_count = 1.0 / float(u.frame_count);
    float2 atlas_uv = float2(in.texcoord.x, in.texcoord.y * inv_count + float(u.frame_index) * inv_count);
    float4 tex_color = tex.sample(samp, atlas_uv);
    float alpha = tex_color.a * u.opacity;
    if (alpha < 0.001) discard_fragment();

    // Rainbow: hue scrolls across the text based on horizontal position and time
    float hue = fract(in.texcoord.x * 0.8 + u.u_time * 0.4);
    float3 rainbow = hsv2rgb_text(hue, 0.8, 1.0);

    return float4(rainbow, alpha);
}
