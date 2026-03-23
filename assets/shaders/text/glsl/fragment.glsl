#version 330
layout (location = 0) out vec4 frag_color;

in vec2 vert_texcoord;
in vec3 vert_color;

uniform sampler2DArray texture_sample;
uniform int frame_index;
uniform float opacity;

void main() {
    vec4 tex_color = texture(texture_sample, vec3(vert_texcoord, float(frame_index)));
    float alpha = tex_color.a * opacity;
    if (alpha < 0.001f) {
        discard;
    }
    frag_color = vec4(vert_color, alpha);
}
