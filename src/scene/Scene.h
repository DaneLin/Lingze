#pragma once

#include "glm/glm.hpp"
#include "scene/Mesh.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "json/json.h"
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
		albedo_color         = glm::vec4(1.0f, 1.0f, 1.0f, 1.f);
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

struct Camera
{
	Camera()
	{
		pos        = glm::vec3(0.0f);
		vert_angle = 0.0f;
		hor_angle  = 0.0f;
	}

	glm::mat4 get_transform_matrix() const
	{
		return glm::translate(pos) * glm::rotate(hor_angle, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::rotate(vert_angle, glm::vec3(1.0f, 0.0f, 0.0f));
	}

	glm::vec3 pos;
	float     vert_angle;
	float     hor_angle;
};

class Scene
{
  public:
	enum struct GeometryTypes : uint8_t
	{
		eTriangles,
		eRegularPoints,
		eSizedPoints
	};

	Scene(Json::Value scene_config, lz::Core *core, GeometryTypes geometry_type);

	// Legacy function: using vertex input binding
	using ObjectCallback = std::function<void(glm::mat4 object_to_world, glm::vec3 albedo_color,
	                                          glm::vec3 emissive_color, vk::Buffer vertex_buffer,
	                                          vk::Buffer index_buffer, uint32_t vertices_count,
	                                          uint32_t indices_count)>;
	void iterate_objects(ObjectCallback object_callback);

	// Create global buffers (CPU side)
	void create_global_buffers();

	// Create buffer for draw call
	void create_draw_buffer();

	Buffer    &get_global_vertex_buffer() const;
	vk::Buffer get_global_index_buffer() const;
	Buffer    &get_draw_call_buffer() const;
	Buffer    &get_draw_indirect_buffer() const;
	Buffer    &get_global_meshlet_buffer() const;
	Buffer    &get_global_meshlet_data_buffer() const;
	size_t     get_global_vertices_count() const
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
	std::vector<std::unique_ptr<Mesh>> meshes_;
	std::vector<Object>                objects_;
	size_t                             marker_object_index_;

	// Global vertex buffer and index buffer
	std::unique_ptr<lz::StagedBuffer> global_vertex_buffer_;
	std::unique_ptr<lz::StagedBuffer> global_index_buffer_;
	std::unique_ptr<lz::StagedBuffer> global_meshlet_buffer_;
	std::unique_ptr<lz::StagedBuffer> global_meshlet_data_buffer_;

	// Draw indirect
	std::unique_ptr<lz::StagedBuffer> draw_call_buffer_;
	std::unique_ptr<lz::StagedBuffer> draw_indirect_buffer_;

	uint32_t global_indices_count_;
	uint32_t global_vertices_count_;
	uint32_t global_meshlet_count_;

	lz::VertexDeclaration vertex_decl_;
	lz::Core             *core_;
};
}        // namespace lz
