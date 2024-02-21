#version 450
layout (location = 0) out vec4 frag_color;

in vec2 vert_texcoord;

uniform sampler2D texture_sample;

void main() {
	vec4 tex_color = texture(texture_sample, vert_texcoord);
	if (tex_color.a < 0.001f) {
		discard;
	}

	frag_color = tex_color;
}