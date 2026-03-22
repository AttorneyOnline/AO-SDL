#version 450
layout (location = 0) out vec4 frag_color;

in vec2 vert_texcoord;

uniform sampler2DArray texture_sample;
uniform int frame_index;
uniform float opacity;

// Text color passed via custom uniforms
uniform float u_text_r;
uniform float u_text_g;
uniform float u_text_b;

void main() {
    vec4 tex_color = texture(texture_sample, vec3(vert_texcoord, float(frame_index)));
    float alpha = tex_color.a * opacity;
    if (alpha < 0.001f) {
        discard;
    }
    frag_color = vec4(u_text_r, u_text_g, u_text_b, alpha);
}
