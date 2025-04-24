#pragma once

#include "scene/Mesh.h"
#include "glm/glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include "json/json.h"
#include <functional>

namespace lz
{
	struct Object
	{
		Object()
		{
			mesh = nullptr;
			obj_to_world = glm::mat4();
			albedo_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.f);
			emissive_color = glm::vec3(1.0f, 1.0f, 1.0f);
			is_shadow_receiver = true;
		}

		Mesh* mesh;
		glm::mat4 obj_to_world;

		glm::vec3 albedo_color;
		glm::vec3 emissive_color;
		bool is_shadow_receiver;
	};

	struct Camera
	{
		Camera()
		{
			pos = glm::vec3(0.0f);
			vert_angle = 0.0f;
			hor_angle = 0.0f;
		}

		glm::mat4 get_transform_matrix() const
		{
			return glm::translate(pos) * glm::rotate(hor_angle, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::rotate(
				vert_angle, glm::vec3(1.0f, 0.0f, 0.0f));
		}


		glm::vec3 pos;
		float vert_angle;
		float hor_angle;
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

		Scene(Json::Value scene_config, lz::Core* core, GeometryTypes geometry_type);

		using ObjectCallback = std::function<void(glm::mat4 object_to_world, glm::vec3 albedo_color,
		                                          glm::vec3 emissive_color, vk::Buffer vertex_buffer,
		                                          vk::Buffer index_buffer, uint32_t vertices_count,
		                                          uint32_t indices_count)>;
		void iterate_objects(ObjectCallback object_callback);

	private:
		std::vector<std::unique_ptr<Mesh>> meshes_;
		std::vector<Object> objects_;
		size_t marker_object_index_;

		lz::VertexDeclaration vertex_decl_;
		lz::Core* core_;
	};
}
