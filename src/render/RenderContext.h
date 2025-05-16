#pragma once

#include "backend/Camera.h"
#include "backend/Config.h"
#include "backend/StagedResources.h"
#include "scene/Entity.h"
#include "scene/Scene.h"
#include "scene/StaticMeshComponent.h"
#include "scene/Transform.h"

#include "glm/glm.hpp"

#include <memory>
#include <vector>

namespace lz::render
{

class lz::Core;
/**
 * @brief Meshlet is used to store the meshlet information for each mesh
 */
struct alignas(16) Meshlet
{
	glm::vec4 sphere_bound;        // bounding sphere, xyz = center, w = radius
	int8_t    cone_axis[3];
	int8_t    cone_cutoff;

	uint32_t data_offset;
	uint32_t vertex_offset;
	uint32_t mesh_draw_index;
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
	uint32_t  vertex_count;         // number of vertices
	uint32_t  index_offset;         // index offset in the buffer
	uint32_t  index_count;          // number of indices
	uint32_t  meshlet_offset;
	uint32_t  meshlet_count;
};

/**
 * @brief MeshDraw is used to store the draw call information for each mesh
 */
#pragma pack(push, 1)
struct MeshDraw
{
	uint32_t  mesh_index;
	uint32_t  material_index;
	float     scale;
	glm::mat4 model_matrix;
};
#pragma pack(pop)

/**
 * @brief Draw command struct
 */
struct DrawCommand
{
	// VkDrawIndexedIndirectCommand
	uint32_t index_count;
	uint32_t instance_count;
	uint32_t first_index;
	uint32_t vertex_offset;
	uint32_t first_instance;

	uint32_t draw_index;
};

struct MeshTaskDrawCommand
{
	// VkDrawMeshTasksIndirectCommandEXT
	uint32_t group_count_x;
	uint32_t group_count_y;
	uint32_t group_count_z;

	uint32_t meshlet_offset;
};

/**
 * @brief Render context class for managing render command generation
 */
class RenderContext
{
  public:
	RenderContext(lz::Core *core);
	~RenderContext();

	/**
	 * @brief Collect rendering commands from the scene
	 * @param scene The scene to render
	 */
	void collect_draw_commands(const lz::Scene *scene);

	/**
	 * @brief Create GPU resources required for rendering
	 */
	void create_gpu_resources();

	/**
	 * @brief Build meshlet data
	 */
	void build_meshlet_data();

	/**
	 * @brief Create meshlet buffer
	 */
	void create_meshlet_buffer();

	lz::Buffer &get_global_vertex_buffer() const
	{
		return global_vertex_buffer_.get()->get_buffer();
	}
	lz::Buffer &get_global_index_buffer() const
	{
		return global_index_buffer_.get()->get_buffer();
	}
	lz::Buffer &get_mesh_draw_buffer() const
	{
		return mesh_draw_buffer_.get()->get_buffer();
	}
	lz::Buffer &get_mesh_info_buffer() const
	{
		return mesh_info_buffer_.get()->get_buffer();
	}
	lz::Buffer &get_mesh_let_buffer() const
	{
		return mesh_let_buffer_.get()->get_buffer();
	}
	lz::Buffer &get_mesh_let_data_buffer() const
	{
		return mesh_let_data_buffer_.get()->get_buffer();
	}

	uint32_t get_draw_count() const
	{
		return draw_count_;
	}

	uint32_t get_meshlet_count() const
	{
		return meshlet_count_;
	}

  private:
	void process_entity(const std::shared_ptr<lz::Entity> &entity);

	lz::Core *core_;

	// Collected draw commands
	uint32_t                          draw_count_ = 0;
	std::vector<MeshInfo>             mesh_infos_;
	std::vector<MeshDraw>             mesh_draws_;
	std::vector<lz::Vertex>           global_vertices_;
	std::vector<uint32_t>             global_indices_;
	std::unique_ptr<lz::StagedBuffer> global_vertex_buffer_;
	std::unique_ptr<lz::StagedBuffer> global_index_buffer_;
	std::unique_ptr<lz::StagedBuffer> mesh_draw_buffer_;
	std::unique_ptr<lz::StagedBuffer> mesh_info_buffer_;

	// Meshlet data
	uint32_t                          meshlet_count_ = 0;
	std::vector<Meshlet>              meshlets_;
	std::vector<uint32_t>             meshlet_data_datum_;
	std::unique_ptr<lz::StagedBuffer> mesh_let_buffer_;
	std::unique_ptr<lz::StagedBuffer> mesh_let_data_buffer_;

	// Statistics
	uint32_t global_vertices_count_ = 0;
	uint32_t global_indices_count_  = 0;
};

}        // namespace lz::render