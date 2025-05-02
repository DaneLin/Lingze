#version 450

#extension GL_EXT_scalar_block_layout : require
#extension GL_ARB_shader_draw_parameters : require
#extension GL_GOOGLE_include_directive : require

#include "Mesh.h"

layout(set = 0, binding = 0, scalar) uniform GlobalData
{
	mat4 view;
	mat4 projection;
};

layout(set = 0, binding = 1, scalar) readonly buffer VertexBuffer
{
    Vertex vertices[];
};

layout(set = 0, binding = 2, scalar) readonly buffer MeshDrawData
{
    MeshDraw mesh_draws[];
};

// storage buffer for draw commands
layout(set = 0, binding = 3) readonly buffer VisibleMeshDrawCommand
{
	MeshDrawCommand draw_cmds[];
};


layout(location = 0) out vec3 out_frag_color;
layout(location = 1) out vec2 out_uv;

void main() {

    uint draw_index = draw_cmds[gl_DrawIDARB].draw_index;
    vec4 pos = vec4(vertices[gl_VertexIndex].pos, 1.0);
    gl_Position = projection * view * mesh_draws[draw_index].model_matrix * pos;

    out_frag_color = vertices[gl_VertexIndex].color;
    out_uv = vertices[gl_VertexIndex].uv;
}