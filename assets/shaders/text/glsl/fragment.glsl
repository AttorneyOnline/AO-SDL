#version 330
layout (location = 0) out vec4 frag_color;

in vec2 vert_texcoord;
in vec3 vert_color;

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

    vec3 color;
    if (vert_color.r < 0.0) {
        // Rainbow: hue based on screen-space X
        float screen_x = gl_FragCoord.x / 512.0;
        float hue = fract(screen_x * 0.8 + u_time * 0.4);
        color = hsv2rgb(hue, 0.8, 1.0);
    } else {
        color = vert_color;
    }

    frag_color = vec4(color, alpha);
}
