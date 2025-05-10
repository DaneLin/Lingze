#pragma once

#include <random>

#include "backend/LingzeVK.h"
#include "backend/StagedResources.h"
#include "backend/VertexDeclaration.h"

namespace lz
{
/**
 * @brief Meshlet is used to store the meshlet information for each mesh
 */
struct Meshlet
{
	uint32_t data_offset;
	uint32_t vertex_offset;
	uint8_t  triangle_count;
	uint8_t  vertex_count;
};

/**
 * @brief MeshInfo is used to store the mesh information for each mesh
 */
struct alignas(16) MeshInfo
{
	glm::vec4 sphere_bound;         // bounding sphere, xyz = center, w = radius
	uint32_t  vertex_offset;        // vertex offset in the buffer
	uint32_t  index_offset;         // index offset in the buffer
	uint32_t  index_count;          // number of indices
};

/**
 * @brief MeshDraw is used to store the draw call information for each mesh
 */
#pragma pack(push, 1)
struct MeshDraw
{
	uint32_t mesh_index;
	glm::mat4 model_matrix;
};
#pragma pack(pop)

/**
 * @brief MeshDrawCommand is used to store the draw call command for each mesh
 */

struct MeshDrawCommand
{
	// VkDrawIndexedIndirectCommand
	uint32_t index_count;
	uint32_t instance_count;
	uint32_t first_index;
	uint32_t vertex_offset;
	uint32_t first_instance;

	uint32_t draw_index;
};

/**
 * @brief Vertex is used to store the vertex information for each mesh
 */
#pragma pack(push, 1)
struct Vertex
{
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 uv;
	};
#pragma pack(pop)

/**
 * @brief MeshData is used to store the mesh data for each mesh
 */
struct MeshData
{
	MeshData();

	MeshData(const std::string file_name, glm::vec3 scale);

	void append_meshlets(std::vector<Meshlet> &meshlets_datum, std::vector<uint32_t> &meshlet_data_datum, const uint32_t vertex_offset);


	static Vertex triangle_vertex_sample(Vertex triangle_vertices[3], glm::vec2 rand_val);

	// bounding sphere
	glm::vec4 sphere_bound;

	// use uint32_t as index type
	using IndexType = uint32_t;
	std::vector<Vertex>    vertices;
	std::vector<IndexType> indices;

	std::vector<Meshlet> meshlets;

	vk::PrimitiveTopology primitive_topology;
};

/**
 * @brief MeshLoader abstract class for loading different mesh formats
 */
class MeshLoader
{
public:
	virtual ~MeshLoader() = default;
	
	/**
	 * @brief Load mesh from file
	 * @param file_name File path
	 * @param scale Scale to apply to the mesh
	 * @return MeshData structure with loaded mesh
	 */
	virtual MeshData load(const std::string& file_name, glm::vec3 scale) = 0;
	
	/**
	 * @brief Check if this loader can handle the given file
	 * @param file_name File path
	 * @return True if this loader can handle the file
	 */
	virtual bool can_load(const std::string& file_name) = 0;
	
	/**
	 * @brief Get a mesh loader for the given file
	 * @param file_name File path
	 * @return Appropriate mesh loader instance
	 */
	static std::shared_ptr<MeshLoader> get_loader(const std::string& file_name);
};

/**
 * @brief ObjLoader for loading OBJ format meshes
 */
class ObjLoader : public MeshLoader
{
public:
	MeshData load(const std::string& file_name, glm::vec3 scale) override;
	bool can_load(const std::string& file_name) override;
};

/**
 * @brief GltfLoader for loading GLTF/GLB format meshes
 */
class GltfLoader : public MeshLoader
{
public:
	MeshData load(const std::string& file_name, glm::vec3 scale) override;
	bool can_load(const std::string& file_name) override;
};

/**
 * @brief Mesh is used to store the mesh data for each mesh
 */
struct Mesh
{
	Mesh(const MeshData &mesh_data);
	     
	Mesh(const std::string &file_name, glm::vec3 scale);
	     
	static lz::VertexDeclaration get_vertex_declaration();


	MeshData mesh_data;
	size_t   indices_count;
	size_t   vertices_count;

	uint32_t global_vertex_offset = 0; // offset in the global vertex buffer
	uint32_t global_index_offset  = 0;  // offset in the global index buffer

	vk::PrimitiveTopology primitive_topology;

	// index of the mesh in the global mesh array
	uint32_t global_mesh_index;
};
}        // namespace lz
