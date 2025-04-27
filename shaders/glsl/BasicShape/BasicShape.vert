#version 450

layout(set = 0, binding = 0) uniform ubo_data
{
	mat4 view;
	mat4 projection;
};

layout(set = 1, binding =0) uniform draw_call_data
{
    mat4 model;
};

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec3 out_frag_color;
layout(location = 1) out vec2 out_uv;

void main() {
    vec4 pos = vec4(in_pos, 1.0);
    gl_Position = projection * view * model * pos;

    out_frag_color = in_color;
    out_uv = in_uv;
}