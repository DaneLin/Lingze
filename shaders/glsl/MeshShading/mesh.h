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

struct Meshlet
{
    uint data_offset;
    uint vertex_offset;
    uint8_t triangleCount;
    uint8_t vertexCount;
};