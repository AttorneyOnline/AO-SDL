struct ShatterFragUniforms {
    int frame_index;
    float opacity;
    float u_time;
};

float shatter_hash(float2 p) {
    return fract(sin(dot(p, float2(127.1, 311.7))) * 43758.5453);
}

float2 shatter_hash2(float2 p) {
    return float2(shatter_hash(p), shatter_hash(p + float2(37.0, 59.0)));
}

fragment float4 fragment_main(VertexOut in [[stage_in]],
                              texture2d_array<float> tex [[texture(0)]],
                              sampler samp [[sampler(0)]],
                              constant ShatterFragUniforms& u [[buffer(0)]]) {
    const float GRID = 16.0;
    float2 uv = in.texcoord;

    float2 cell = floor(uv * GRID);
    float2 local_uv = fract(uv * GRID);

    float diag_dir = step(0.5, shatter_hash(cell * 0.73));
    float in_upper;
    if (diag_dir > 0.5)
        in_upper = step(local_uv.x, local_uv.y);
    else
        in_upper = step(1.0 - local_uv.x, local_uv.y);

    float2 tri_id = cell * 2.0 + float2(in_upper, diag_dir);

    float2 shard_center = (cell + 0.5) / GRID;
    float2 from_img_center = shard_center - float2(0.5);
    float2 rand_dir = normalize(from_img_center + (shatter_hash2(tri_id) - 0.5) * 0.6);
    float rand_speed = 0.8 + shatter_hash(tri_id + float2(43.0, 71.0)) * 1.2;

    float t = clamp(u.u_time * 0.4, 0.0, 1.0);
    float2 offset = rand_dir * t * rand_speed * 0.8;

    float2 sample_uv = uv - offset;

    if (sample_uv.x < 0.0 || sample_uv.x > 1.0 || sample_uv.y < 0.0 || sample_uv.y > 1.0)
        discard_fragment();

    float4 color = tex.sample(samp, sample_uv, u.frame_index);
    color.a *= u.opacity;

    if (color.a < 0.001)
        discard_fragment();

    return color;
}
