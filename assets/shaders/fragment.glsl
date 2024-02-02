#version 450
layout (location = 0) out vec4 frag_color;

in vec3 vert_albedo;
in vec2 vert_texcoord;

uniform sampler2D texture_sample;

void main() {
	vec4 tex_color = texture(texture_sample, vert_texcoord);
	frag_color = vec4(tex_color.xyz, 1.0f);
}