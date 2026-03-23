#version 330
layout (location = 0) in vec2 vertex_pos;
layout (location = 1) in vec2 vertex_tex_coord;
layout (location = 2) in vec3 vertex_color;

out vec2 vert_texcoord;
out vec3 vert_color;

uniform mat4 local;
uniform float aspect;

void main() {
    gl_Position = local * vec4(vertex_pos.x, vertex_pos.y, 0.0f, 1.0f);
    vert_texcoord = vertex_tex_coord;
    vert_color = vertex_color;
}
