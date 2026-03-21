#version 450
layout (location = 0) out vec4 frag_color;

in vec2 vert_texcoord;

uniform sampler2DArray texture_sample;
uniform int frame_index;
uniform float opacity;
uniform float u_time;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

vec2 hash2(vec2 p) {
    return vec2(hash(p), hash(p + vec2(37.0, 59.0)));
}

void main() {
    const float GRID = 16.0;
    vec2 uv = vert_texcoord;

    // Determine which triangle shard this fragment belongs to
    vec2 cell = floor(uv * GRID);
    vec2 local_uv = fract(uv * GRID);

    float diag_dir = step(0.5, hash(cell * 0.73));
    float in_upper;
    if (diag_dir > 0.5)
        in_upper = step(local_uv.x, local_uv.y);
    else
        in_upper = step(1.0 - local_uv.x, local_uv.y);

    vec2 tri_id = cell * 2.0 + vec2(in_upper, diag_dir);

    // Random 2D direction per shard (normalized-ish, biased outward from center)
    vec2 shard_center = (cell + 0.5) / GRID;
    vec2 from_img_center = shard_center - vec2(0.5);
    vec2 rand_dir = normalize(from_img_center + (hash2(tri_id) - 0.5) * 0.6);
    float rand_speed = 0.8 + hash(tri_id + vec2(43.0, 71.0)) * 1.2;

    // Animation: shards slide outward in 2D
    float t = clamp(u_time * 0.4, 0.0, 1.0);
    vec2 offset = rand_dir * t * rand_speed * 0.8;

    // Sample texture at the reverse-mapped position (where this pixel came from)
    vec2 sample_uv = uv - offset;

    // Discard if the source is outside the texture
    if (sample_uv.x < 0.0 || sample_uv.x > 1.0 || sample_uv.y < 0.0 || sample_uv.y > 1.0)
        discard;

    vec4 tex_color = texture(texture_sample, vec3(sample_uv, float(frame_index)));
    tex_color.a *= opacity;

    if (tex_color.a < 0.001)
        discard;

    frag_color = tex_color;
}
