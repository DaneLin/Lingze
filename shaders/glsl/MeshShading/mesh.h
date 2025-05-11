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
	uint8_t triangle_count;
	uint8_t vertex_count;
};