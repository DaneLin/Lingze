#pragma once

#include "glm/glm.hpp"
#include "scene/Mesh.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "tiny_gltf.h"        // Include tiny_gltf header
#include "json/json.h"
#include <filesystem>        // Adding for filesystem path operations
#include <functional>
#include <glm/gtx/transform.hpp>

namespace lz
{
struct Object
{
	Object()
	{
		mesh                 = nullptr;
		obj_to_world         = glm::mat4();
		albedo_color         = glm::vec3(1.0f, 1.0f, 1.0f);
		emissive_color       = glm::vec3(1.0f, 1.0f, 1.0f);
		is_shadow_receiver   = true;
		global_vertex_offset = 0;
		global_index_offset  = 0;
	}

	Mesh     *mesh;
	glm::mat4 obj_to_world;

	glm::vec3 albedo_color;
	glm::vec3 emissive_color;
	bool      is_shadow_receiver;

	// Offset in global buffers
	uint32_t global_vertex_offset;
	uint32_t global_index_offset;
};

class JsonScene
{
  public:
	enum struct GeometryTypes : uint8_t
	{
		eTriangles,
		eRegularPoints,
		eSizedPoints
	};

	// Constructor for JSON scene config
	JsonScene(Json::Value scene_config, lz::Core *core, GeometryTypes geometry_type);

	// New constructor for GLTF file loading
	JsonScene(const std::string &file_path, lz::Core *core, GeometryTypes geometry_type);

	// Create global buffers (CPU side)
	void create_global_buffers(bool build_meshlet = false);

	// Create indirect buffer for draw call (CPU side)
	void create_draw_buffer();

	// Create necessary buffers for compute shader to generate draw commands
	void create_draw_call_info_buffer();

	Buffer    &get_global_vertex_buffer() const;
	vk::Buffer get_global_index_buffer() const;
	Buffer    &get_draw_call_buffer() const;
	Buffer    &get_draw_indirect_buffer() const;
	Buffer    &get_draw_call_info_buffer() const;
	Buffer    &get_global_meshlet_buffer() const;
	Buffer    &get_global_meshlet_data_buffer() const;

	Buffer &get_global_mesh_info_buffer() const;
	Buffer &get_global_mesh_draw_buffer() const;

	size_t get_global_vertices_count() const
	{
		return global_vertices_count_;
	}
	size_t get_global_indices_count() const
	{
		return global_indices_count_;
	}
	size_t get_global_meshlet_count() const
	{
		return global_meshlet_count_;
	}

	size_t get_draw_count() const
	{
		return objects_.size();
	}

  private:
	// Helper method to load meshes and create objects from a GLTF file
	void load_from_gltf(const std::string &file_path);

	// Helper to recursively process GLTF nodes
	void process_node(const tinygltf::Model &model, int node_idx,
	                  const std::map<int, Mesh *> &mesh_map,
	                  const glm::mat4             &parent_transform);

	std::vector<std::unique_ptr<Mesh>> meshes_;
	std::vector<Object>                objects_;
	size_t                             marker_object_index_;

	// Global vertex buffer and index buffer
	std::unique_ptr<lz::StagedBuffer> global_vertex_buffer_;
	std::unique_ptr<lz::StagedBuffer> global_index_buffer_;
	std::unique_ptr<lz::StagedBuffer> draw_call_buffer_;
	std::unique_ptr<lz::StagedBuffer> draw_indirect_buffer_;

	// Compute Shader buffer
	std::unique_ptr<lz::StagedBuffer> draw_call_info_buffer_;

	// Meshlet buffer
	std::unique_ptr<lz::StagedBuffer> global_meshlet_buffer_;
	std::unique_ptr<lz::StagedBuffer> global_meshlet_data_buffer_;

	// Buffer for geometry shader to generate draw commands
	std::unique_ptr<lz::StagedBuffer> global_mesh_info_buffer_;
	std::unique_ptr<lz::StagedBuffer> global_mesh_draw_buffer_;

	// Meta data
	size_t global_indices_count_  = 0;
	size_t global_vertices_count_ = 0;
	size_t global_meshlet_count_  = 0;

	// Vertex format
	lz::VertexDeclaration vertex_decl_;

	lz::Core *core_ = nullptr;
};
}        // namespace lz
