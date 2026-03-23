struct TextFragUniforms {
    int frame_index;
    float opacity;
    int frame_count;
};

fragment float4 fragment_main(VertexOut in [[stage_in]],
                              texture2d<float> tex [[texture(0)]],
                              sampler samp [[sampler(0)]],
                              constant TextFragUniforms& u [[buffer(0)]]) {
    float inv_count = 1.0 / float(u.frame_count);
    float2 atlas_uv = float2(in.texcoord.x, in.texcoord.y * inv_count + float(u.frame_index) * inv_count);
    float4 tex_color = tex.sample(samp, atlas_uv);
    float alpha = tex_color.a * u.opacity;
    if (alpha < 0.001) discard_fragment();
    return float4(in.color, alpha);
}
