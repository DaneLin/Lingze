#pragma once

#include "backend/LingzeVK.h"
#include "backend/VertexDeclaration.h"
#include <memory>
#include <string>
#include <vector>

namespace lz
{
/**
 * @brief Texture is a class that contains image data
 */
class Texture
{
  public:
	Texture()  = default;
	~Texture() = default;

	int                        width{-1};           // texture width
	int                        height{-1};          // texture height
	int                        channels{-1};        // number of color channels
	std::vector<unsigned char> data;                // image data
	std::string                name;                // texture name
	std::string                uri;                 // texture URI or file path
};

/**
 * @brief Material is a material class that contains different types of texture maps
 */
class Material
{
  public:
	Material()  = default;
	~Material() = default;

	std::string              name;                              // material name
	std::shared_ptr<Texture> diffuse_texture;                   // diffuse texture
	std::shared_ptr<Texture> normal_texture;                    // normal texture
	std::shared_ptr<Texture> metallic_roughness_texture;        // metallic roughness texture
	std::shared_ptr<Texture> emissive_texture;                  // emissive texture
	std::shared_ptr<Texture> occlusion_texture;                 // occlusion texture

	// PBR parameters
	glm::vec4 base_color_factor{1.0f};
	float     metallic_factor{1.0f};
	float     roughness_factor{1.0f};
	glm::vec3 emissive_factor{0.0f};
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
	std::vector<Vertex>       vertices;
	std::vector<uint32_t>     indices;
	vk::PrimitiveTopology     primitive_topology;
	glm::vec4                 sphere_bound;
	std::string               material_name;
	std::shared_ptr<Material> material;        // 添加对材质的引用
};

/**
 * @brief Mesh is a 3D model composed of multiple SubMeshes
 */
class Mesh
{
  public:
	Mesh();

	Mesh(const std::string &file_name);

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

	// add material management function
	void                                          add_material(const std::shared_ptr<Material> &material);
	std::shared_ptr<Material>                     get_material(const std::string &name);
	const std::vector<std::shared_ptr<Material>> &get_materials() const;

  private:
	std::vector<SubMesh>                   sub_meshes;
	glm::vec4                              mesh_bound;
	std::vector<std::shared_ptr<Material>> materials;        // 存储所有材质
};

}        // namespace lz
