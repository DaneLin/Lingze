#define DEBUG 0

layout(set = 0, binding = 0) uniform ubo_data
{
	mat4 view;
	mat4 projection;
};

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





