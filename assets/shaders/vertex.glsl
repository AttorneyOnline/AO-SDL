#version 450
layout (location = 0) in vec2 vertex_pos;
layout (location = 1) in vec2 vertex_tex_coord;

out vec2 vert_texcoord;

uniform mat4 local;
uniform float aspect;

void main() {
    mat4 aspect_mat = mat4(
        vec4(1.0/aspect, 0.0, 0.0, 0.0), 
        vec4(0.0,        1.0, 0.0, 0.0), 
        vec4(0.0,        0.0, 1.0, 0.0), 
        vec4(0.0,        0.0, 0.0, 1.0)); 

    gl_Position = local * vec4(vertex_pos.x, vertex_pos.y, 0.0f, 1.0f);
    vert_texcoord = vertex_tex_coord;
}