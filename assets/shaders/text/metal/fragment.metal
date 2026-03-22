// Uniform fields after frame_index and opacity are packed alphabetically
// by key name from the ShaderUniformProvider.
struct TextFragUniforms {
    int frame_index;
    float opacity;
    float u_text_b; // alphabetical: b < g < r
    float u_text_g;
    float u_text_r;
};

fragment float4 fragment_main(VertexOut in [[stage_in]],
                              texture2d_array<float> tex [[texture(0)]],
                              sampler samp [[sampler(0)]],
                              constant TextFragUniforms& u [[buffer(0)]]) {
    float4 tex_color = tex.sample(samp, in.texcoord, u.frame_index);
    float alpha = tex_color.a * u.opacity;
    if (alpha < 0.001) discard_fragment();
    return float4(u.u_text_r, u.u_text_g, u.u_text_b, alpha);
}
