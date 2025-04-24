#include "Scene.h"

#include "backend/Core.h"
#include "backend/PresentQueue.h"


namespace lz
{
	glm::vec3 ReadJsonVec3f(Json::Value vectorValue)
	{
		return glm::vec3(vectorValue[0].asFloat(), vectorValue[1].asFloat(), vectorValue[2].asFloat());
	}

	glm::ivec2 ReadJsonVec2i(Json::Value vectorValue)
	{
		return glm::ivec2(vectorValue[0].asInt(), vectorValue[1].asInt());
	}

	glm::uvec3 ReadJsonVec3u(Json::Value vectorValue)
	{
		return glm::uvec3(vectorValue[0].asUInt(), vectorValue[1].asUInt(), vectorValue[2].asUInt());
	}

	Scene::Scene(Json::Value scene_config, lz::Core* core, GeometryTypes geometry_type)
	{
		this->core_ = core;

		vertex_decl_ = Mesh::get_vertex_declaration();
		lz::ExecuteOnceQueue transfer_queue(core);

		std::map<std::string, Mesh*> name_to_mesh;

		auto transfer_command_buffer = transfer_queue.begin_command_buffer();
		{
			Json::Value mesh_array = scene_config["meshes"];
			for (Json::ArrayIndex mesh_index = 0; mesh_index < mesh_array.size(); ++mesh_index)
			{
				Json::Value curr_mesh_node = mesh_array[mesh_index];

				std::string mesh_file_name = curr_mesh_node.get("filename", "<unspecified>").asString();
				glm::vec3 scale = ReadJsonVec3f(curr_mesh_node["scale"]);

				auto mesh_data = MeshData(mesh_file_name, scale);
				switch (geometry_type)
				{
				case GeometryTypes::eRegularPoints:
					{
						float splat_size = 0.1f;
						mesh_data = MeshData::generate_point_mesh_regular(mesh_data, std::pow(1.0f / splat_size, 2.0f));
					}
					break;
				case GeometryTypes::eSizedPoints:
					{
						mesh_data = MeshData::generate_point_mesh_sized(mesh_data, 1);
					}
					break;
				}

				auto mesh = std::unique_ptr<Mesh>(new Mesh(mesh_data, core_->get_physical_device(), core_->get_logical_device(), transfer_command_buffer));
				meshes_.push_back(std::move(mesh));

				std::string mesh_name = curr_mesh_node.get("name", "<unspecified>").asString();
				name_to_mesh[mesh_name] = meshes_.back().get();
			}
		}
		transfer_queue.end_command_buffer();

		for (Json::ArrayIndex object_index = 0; object_index < scene_config["objects"].size(); ++object_index)
		{
			Object object;

			Json::Value curr_object_node = scene_config["objects"][object_index];
			std::string mesh_name = curr_object_node.get("mesh", "<unspecified>").asString();
			if (name_to_mesh.find(mesh_name) == name_to_mesh.end())
			{
				std::cout << "Mesh " << mesh_name << " not specified\n";
				continue;
			}

			object.mesh = name_to_mesh[mesh_name];
			glm::vec3 rotation_vec = ReadJsonVec3f(curr_object_node["angle"]);
			object.obj_to_world = glm::translate(ReadJsonVec3f(curr_object_node["pos"]));
			if (glm::length(rotation_vec) > 1e-3f)
			{
				object.obj_to_world = object.obj_to_world * glm::rotate(glm::length(rotation_vec), glm::normalize(rotation_vec));
			}

			object.albedo_color = ReadJsonVec3f(curr_object_node["albedoColor"]);
			object.emissive_color = ReadJsonVec3f(curr_object_node["emissiveColor"]);
			object.is_shadow_receiver = curr_object_node.get("isShadowCaster", true).asBool();

			if (curr_object_node.get("isMarker", false).asBool())
			{
				marker_object_index_ = objects_.size();
			}

			objects_.push_back(object);
		}
	}

	void Scene::IterateObjects(ObjectCallback object_callback)
	{
		for (auto& object : objects_)
		{
			object_callback(object.obj_to_world, object.albedo_color, object.emissive_color,
			                object.mesh->vertex_buffer->get_buffer(),
			                object.mesh->index_buffer ? object.mesh->index_buffer->get_buffer() : nullptr,
			                uint32_t(object.mesh->vertices_count), uint32_t(object.mesh->indices_count));
		}
	}
}
