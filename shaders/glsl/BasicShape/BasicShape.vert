#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec3 out_frag_color;
layout(location = 1) out vec2 out_uv;

void main() {
    gl_Position = vec4(in_pos, 1.0);
    out_frag_color = in_color;
    out_uv = in_uv;
}