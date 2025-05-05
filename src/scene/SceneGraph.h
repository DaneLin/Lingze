#pragma once

#include <cmath>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>


#include "glm/glm.hpp"
#include "scene/Mesh.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

namespace lz
{
// Forward declarations
class Core;
class Buffer;

/**
 * @brief Node represents a node in the scene graph with transformation and optionally a mesh
 */
struct Node
{
	std::string         name;
	glm::mat4           local_transform  = glm::mat4(1.0f);
	glm::mat4           global_transform = glm::mat4(1.0f);
	std::vector<size_t> submesh_indices;
	std::vector<size_t> children;
	size_t              parent  = std::numeric_limits<size_t>::max();
	bool                visible = true;
};

/**
 * @brief SubMesh represents a portion of a mesh with its own material
 */
struct SubMesh
{
	size_t    mesh_index;                    // Index to the parent mesh
	size_t    index_offset       = 0;        // Offset into the index buffer
	size_t    index_count        = 0;        // Number of indices
	glm::vec3 albedo_color       = glm::vec3(1.0f);
	glm::vec3 emissive_color     = glm::vec3(0.0f);
	bool      is_shadow_receiver = true;
};

/**
 * @brief DrawCommand contains information needed for rendering
 */
struct DrawCommand
{
	size_t    node_index;                  // Index to the node
	size_t    submesh_index;               // Index to the submesh
	glm::mat4 transform;                   // Global transform
	uint32_t  global_vertex_offset;        // Offset in global vertex buffer
	uint32_t  global_index_offset;         // Offset in global index buffer
};

/**
 * @brief SceneGraph manages hierarchical scene representation with GLTF models
 */
class SceneGraph
{
  public:
	/**
	 * @brief Construct a scene graph
	 * @param core Lingze core instance
	 */
	explicit SceneGraph(lz::Core *core);

	/**
	 * @brief Destroy the scene graph
	 */
	~SceneGraph();

	/**
	 * @brief Load a GLTF model file
	 * @param filepath Path to the GLTF file
	 * @param scale Scale to apply to the model
	 * @return Root node index
	 */
	size_t load_model(const std::string &filepath, const glm::vec3 &scale = glm::vec3(1.0f));

	/**
	 * @brief Update scene graph transforms
	 */
	void update_transforms();

	/**
	 * @brief Create global vertex and index buffers
	 */
	void create_global_buffers();

	/**
	 * @brief Create draw commands for rendering
	 */
	void create_draw_commands();

	/**
	 * @brief Get all draw commands for rendering
	 * @return Vector of draw commands
	 */
	const std::vector<DrawCommand> &get_draw_commands() const;

	/**
	 * @brief Get a node by its index
	 * @param index Node index
	 * @return Reference to the node
	 */
	Node &get_node(size_t index);

	/**
	 * @brief Get a node by name
	 * @param name Node name
	 * @return Node index or max size_t if not found
	 */
	size_t find_node(const std::string &name) const;

	/**
	 * @brief Set node visibility
	 * @param node_index Node index
	 * @param visible Visibility flag
	 */
	void set_node_visibility(size_t node_index, bool visible);

	/**
	 * @brief Get global vertex buffer
	 * @return Reference to the buffer
	 */
	Buffer &get_global_vertex_buffer() const;

	/**
	 * @brief Get global index buffer
	 * @return Vulkan buffer handle
	 */
	vk::Buffer get_global_index_buffer() const;

	/**
	 * @brief Get draw indirect buffer
	 * @return Reference to the buffer
	 */
	Buffer &get_draw_indirect_buffer() const;

	/**
	 * @brief Get total number of vertices
	 * @return Vertex count
	 */
	size_t get_global_vertices_count() const
	{
		return global_vertices_count_;
	}

	/**
	 * @brief Get total number of indices
	 * @return Index count
	 */
	size_t get_global_indices_count() const
	{
		return global_indices_count_;
	}

	/**
	 * @brief Get number of draw commands
	 * @return Draw command count
	 */
	size_t get_draw_count() const
	{
		return draw_commands_.size();
	}

	/**
	 * @brief Clear the scene graph
	 */
	void clear();

  private:
	/**
	 * @brief Update global transform for a node and its children
	 * @param node_index Index of the node to update
	 * @param parent_transform Parent's global transform
	 */
	void update_node_transform(size_t node_index, const glm::mat4 &parent_transform);

	/**
	 * @brief Generate draw commands for visible nodes
	 */
	void generate_draw_commands();

	std::vector<std::unique_ptr<Mesh>> meshes_;
	std::vector<Node>                  nodes_;
	std::vector<SubMesh>               submeshes_;
	std::vector<DrawCommand>           draw_commands_;

	std::unordered_map<std::string, size_t> name_to_node_;

	// Global buffers
	std::unique_ptr<lz::StagedBuffer> global_vertex_buffer_;
	std::unique_ptr<lz::StagedBuffer> global_index_buffer_;
	std::unique_ptr<lz::StagedBuffer> draw_indirect_buffer_;

	uint32_t global_vertices_count_ = 0;
	uint32_t global_indices_count_  = 0;
	bool     buffers_created_       = false;
	bool     commands_dirty_        = true;

	lz::Core *core_;
};

}        // namespace lz