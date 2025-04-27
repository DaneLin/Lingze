#version 450

#extension GL_EXT_scalar_block_layout : require
#extension GL_ARB_shader_draw_parameters : require

layout(set = 0, binding = 0) uniform ubo_data
{
	mat4 view;
	mat4 projection;
};

struct Vertex
{
    vec3 pos;
    vec3 color;
    vec2 uv;
};

layout(set = 0, binding = 1, scalar) readonly buffer VertexBuffer
{
    Vertex vertices[];
};

struct DrawCallData
{
	mat4 model;
};


layout(set = 0, binding = 2, scalar) readonly buffer DrawCallDataBuffer
{
    DrawCallData draw_calls[];
};


layout(location = 0) out vec3 out_frag_color;
layout(location = 1) out vec2 out_uv;

void main() {
    vec4 pos = vec4(vertices[gl_VertexIndex].pos, 1.0);
    gl_Position = projection * view * draw_calls[gl_DrawIDARB].model * pos;

    out_frag_color = vertices[gl_VertexIndex].color;
    out_uv = vertices[gl_VertexIndex].uv;
}