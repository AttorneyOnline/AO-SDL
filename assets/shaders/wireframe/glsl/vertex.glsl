#version 330
layout (location = 0) in vec2 vertex_pos;
layout (location = 1) in vec2 vertex_tex_coord;
layout (location = 2) in vec3 vertex_color;
out vec3 vert_color;
uniform mat4 local;
void main() { gl_Position = local * vec4(vertex_pos, 0.0, 1.0); vert_color = vertex_color; }
