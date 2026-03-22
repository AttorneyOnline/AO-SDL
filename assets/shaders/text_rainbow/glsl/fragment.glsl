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
    float alpha = tex_color.a * opacity;
    if (alpha < 0.001f) {
        discard;
    }

    // Rainbow: hue scrolls across the text based on horizontal position and time
    float hue = fract(vert_texcoord.x * 0.8 + u_time * 0.4);
    vec3 rainbow = hsv2rgb(hue, 0.8, 1.0);

    frag_color = vec4(rainbow, alpha);
}
