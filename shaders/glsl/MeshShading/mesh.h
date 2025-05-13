#define DEBUG 0
#define CULL 1

#include "../../../src/config/EngineConfig.h"

struct Vertex
{
	vec3 pos;
	vec3 normal;
	vec2 uv;
};

struct Meshlet
{
	vec4    sphere_bound;
	int8_t  cone_axis[3];
	int8_t  cone_cutoff;
	uint    data_offset;
	uint    vertex_offset;
	uint    material_index;
	uint8_t triangle_count;
	uint8_t vertex_count;
};

struct CullData
{
	mat4  view_matrix;                  // view matrix, for converting world coordinates to view coordinates
	float P00, P11, znear, zfar;        // symmetirc projection parameters
	float frustum[4];                   // data for left / right / top / bottom
	uint  draw_count;                   // number of draw commands
};

struct MeshInfo
{
	vec4 sphere_bound;         // bounding sphere, xyz = center, w = radius
	uint vertex_offset;        // vertex offset in the buffer
	uint vertex_count;         // vertex offset in the buffer
	uint index_offset;         // index offset in the buffer
	uint index_count;          // number of indices
    uint meshlet_offset;       // meshlet offset in the buffer
	uint meshlet_count;        // number of meshlets
};

struct MeshDraw
{
	uint mesh_index;        // index of the mesh in the mesh array
	uint material_index;
	mat4 model_matrix;
};

struct MaterialParameters
{
	vec4  base_color_factor;
	vec3  emissive_factor;
	float metallic_factor;
	float roughness_factor;
	uint  diffuse_texture_index;
	uint  normal_texture_index;
	uint  metallic_roughness_texture_index;
	uint  emissive_texture_index;
	uint  occlusion_texture_index;
};

// Task payload for meshlet culling
struct TaskPayload
{
	uint meshlet_indices[TASK_WGSIZE];
};

struct MeshTaskDrawCommand
{
	uint group_count_x;
	uint group_count_y;
	uint group_count_z;

	uint meshlet_offset;
};
