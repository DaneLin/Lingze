struct Vertex
{
	vec3 pos;
	vec3 color;
	vec2 uv;
};

struct CullData
{
	float P00, P11, znear, zfar;        // symmetirc projection parameters
	float frustum[4];                   // data for left / right / top / bottom
	uint  draw_count;        // number of draw commands
};

struct Mesh
{
	vec4 sphere_bound;         // bounding sphere, xyz = center, w = radius
	uint vertex_offset;        // vertex offset in the buffer
	uint index_offset;         // index offset in the buffer
	uint index_count;          // number of indices
};

struct MeshDraw
{
	uint mesh_index;        // index of the mesh in the mesh array
	mat4 model_matrix;
};

struct MeshDrawCommand
{
	// VkDrawIndexedIndirectCommand
	uint index_count;
	uint instance_count;
	uint first_index;
	uint vertex_offset;
	uint first_instance;
	
	uint draw_index;
};