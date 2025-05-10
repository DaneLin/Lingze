#pragma once

#include "backend/LingzeVK.h"
#include "backend/VertexDeclaration.h"
#include <memory>
#include <string>
#include <vector>

namespace lz
{
/**
 * @brief Vertex is used to store the vertex information for each mesh
 */
#pragma pack(push, 1)
struct LzVertex
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 uv;
};
#pragma pack(pop)

/**
 * @brief SubMesh is part of a Mesh
 */
class SubMesh
{
  public:
	SubMesh() :
	    primitive_topology(vk::PrimitiveTopology::eTriangleList)
	{}

	size_t get_vertex_count() const
	{
		return vertices.size();
	}
	size_t get_index_count() const
	{
		return indices.size();
	}
	glm::vec4 calculate_bounding_sphere();
	void      optimize();

  public:
	std::vector<LzVertex> vertices;
	std::vector<uint32_t> indices;
	vk::PrimitiveTopology primitive_topology;
	glm::vec4             sphere_bound;
	std::string           material_name;
};

/**
 * @brief LzMesh is a 3D model composed of multiple SubMeshes
 */
class LzMesh
{
  public:
	LzMesh();

	LzMesh(const std::string &file_name);

	void add_sub_mesh(const SubMesh &sub_mesh);

	size_t get_sub_mesh_count() const
	{
		return sub_meshes.size();
	}

	SubMesh &get_sub_mesh(size_t index);

	const SubMesh &get_sub_mesh(size_t index) const;

	glm::vec4 calculate_bounding_sphere();

	static lz::VertexDeclaration get_vertex_declaration();

	void optimize();

	size_t get_total_vertex_count() const;
	size_t get_total_index_count() const;

  private:
	std::vector<SubMesh> sub_meshes;
	glm::vec4            mesh_bound;
};

}        // namespace lz
