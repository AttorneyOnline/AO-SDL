#version 330
layout (location = 0) out vec4 frag_color;

in vec2 vert_texcoord;

uniform sampler2DArray texture_sample;
uniform int frame_index;
uniform float opacity;
uniform float u_time;

vec3 hsv2rgb(float h, float s, float v) {
    vec3 c = vec3(h, s, v);
    vec3 rgb = clamp(abs(mod(c.x * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    return c.z * mix(vec3(1.0), rgb, c.y);
}

void main() {
    vec4 tex_color = texture(texture_sample, vec3(vert_texcoord, float(frame_index)));
    tex_color.a *= opacity;
    if (tex_color.a < 0.001f) {
        discard;
    }

    // Rainbow overlay: scrolls diagonally over time
    float hue = fract((vert_texcoord.x + vert_texcoord.y) * 0.5 + u_time * 0.3);
    vec3 rainbow = hsv2rgb(hue, 0.6, 1.0);

    // Blend: mix original color with rainbow (40% rainbow, 60% original)
    tex_color.rgb = mix(tex_color.rgb, rainbow, 0.4);

    frag_color = tex_color;
}
