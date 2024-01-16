#version 450
layout (location = 0) out vec4 frag_color;

in vec3 vert_albedo;
in vec2 vert_texcoord;

uniform sampler2D texture_sample;

void main() {
	// frag_color = vec4(vert_albedo.x, vert_albedo.y, vert_albedo.z, 1.0f);
	frag_color = vec4(vert_texcoord, 0.0f, 1.0f);
	//frag_color = texture(texture_sample, vert_texcoord);
}