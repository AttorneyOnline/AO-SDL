#version 450
layout (location = 0) in vec2 vertex_pos;
layout (location = 1) in vec2 vertex_tex_coord;
uniform mat4 local;
void main() { gl_Position = local * vec4(vertex_pos, 0.0, 1.0); }
