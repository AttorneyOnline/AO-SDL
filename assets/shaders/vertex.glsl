#version 450
layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in vec3 vertex_color;
layout (location = 2) in vec2 vertex_tex_coord;

out vec3 vert_albedo;
out vec2 vert_texcoord;

void main() {
    gl_Position = vec4(vertex_pos.x, vertex_pos.y, vertex_pos.z, 1);
    vert_albedo = vertex_color;
    vert_texcoord = vertex_tex_coord;
}