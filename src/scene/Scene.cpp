#include "Scene.h"

#include "backend/Logging.h"
#include "backend/Core.h"
#include "backend/PresentQueue.h"

namespace lz
{
static glm::vec3 read_json_vec3_f(Json::Value vectorValue)
{
	return glm::vec3(vectorValue[0].asFloat(), vectorValue[1].asFloat(), vectorValue[2].asFloat());
}

static glm::ivec2 read_json_vec2_i(Json::Value vectorValue)
{
	return glm::ivec2(vectorValue[0].asInt(), vectorValue[1].asInt());
}

static glm::uvec3 read_json_vec3_u(Json::Value vectorValue)
{
	return glm::uvec3(vectorValue[0].asUInt(), vectorValue[1].asUInt(), vectorValue[2].asUInt());
}

Scene::Scene(Json::Value scene_config, lz::Core *core, GeometryTypes geometry_type)
{
	this->core_ = core;

	vertex_decl_ = Mesh::get_vertex_declaration();
	lz::ExecuteOnceQueue transfer_queue(core);

	std::map<std::string, Mesh *> name_to_mesh;

	auto transfer_command_buffer = transfer_queue.begin_command_buffer();
	{
		Json::Value &mesh_array = scene_config["meshes"];
		for (Json::ArrayIndex mesh_index = 0; mesh_index < mesh_array.size(); ++mesh_index)
		{
			Json::Value &curr_mesh_node = mesh_array[mesh_index];

			std::string mesh_file_name = curr_mesh_node.get("filename", "<unspecified>").asString();
			glm::vec3   scale          = read_json_vec3_f(curr_mesh_node["scale"]);

			auto mesh_data = MeshData(mesh_file_name, scale);
			switch (geometry_type)
			{
				case GeometryTypes::eRegularPoints:
				{
					float splat_size = 0.1f;
					mesh_data        = MeshData::generate_point_mesh_regular(mesh_data, std::pow(1.0f / splat_size, 2.0f));
				}
				break;
				case GeometryTypes::eSizedPoints:
				{
					mesh_data = MeshData::generate_point_mesh_sized(mesh_data, 1);
				}
				break;
			}

			auto mesh               = std::unique_ptr<Mesh>(new Mesh(mesh_data, core_->get_physical_device(), core_->get_logical_device(), transfer_command_buffer));
			mesh->global_mesh_index = uint32_t(meshes_.size());
			meshes_.push_back(std::move(mesh));

			std::string mesh_name   = curr_mesh_node.get("name", "<unspecified>").asString();
			name_to_mesh[mesh_name] = meshes_.back().get();
		}
	}
	transfer_queue.end_command_buffer();

	for (Json::ArrayIndex object_index = 0; object_index < scene_config["objects"].size(); ++object_index)
	{
		Object object;

		Json::Value curr_object_node = scene_config["objects"][object_index];
		std::string mesh_name        = curr_object_node.get("mesh", "<unspecified>").asString();
		if (name_to_mesh.find(mesh_name) == name_to_mesh.end())
		{
			LOGW("Mesh {} not specified", mesh_name);
			continue;
		}

		object.mesh            = name_to_mesh[mesh_name];
		glm::vec3 rotation_vec = read_json_vec3_f(curr_object_node["angle"]);
		object.obj_to_world    = glm::translate(read_json_vec3_f(curr_object_node["pos"]));
		if (glm::length(rotation_vec) > 1e-3f)
		{
			object.obj_to_world = object.obj_to_world * glm::rotate(glm::length(rotation_vec), glm::normalize(rotation_vec));
		}

		object.albedo_color       = read_json_vec3_f(curr_object_node["albedoColor"]);
		object.emissive_color     = read_json_vec3_f(curr_object_node["emissiveColor"]);
		object.is_shadow_receiver = curr_object_node.get("isShadowCaster", true).asBool();

		if (curr_object_node.get("isMarker", false).asBool())
		{
			marker_object_index_ = objects_.size();
		}

		objects_.push_back(object);
	}
}

void Scene::iterate_objects(ObjectCallback object_callback)
{
	for (auto &object : objects_)
	{
		object_callback(object.obj_to_world, object.albedo_color, object.emissive_color,
		                object.mesh->vertex_buffer->get_buffer().get_handle(),
		                object.mesh->index_buffer ? object.mesh->index_buffer->get_buffer().get_handle() : nullptr,
		                uint32_t(object.mesh->vertices_count), uint32_t(object.mesh->indices_count));
	}
}

void Scene::create_global_buffers(bool build_meshlet)
{
	// calc total vertex and index offset
	uint32_t total_vertex_size = 0;
	uint32_t total_index_size  = 0;
	global_vertices_count_     = 0;
	global_indices_count_      = 0;

	// use a map to record each mesh's offset in global buffer
	std::unordered_map<Mesh *, std::pair<uint32_t, uint32_t>> mesh_to_offsets;

	std::vector<MeshInfo> all_mesh_infos;
	all_mesh_infos.resize(meshes_.size());

	std::vector<Meshlet>  all_meshlets;
	std::vector<uint32_t> all_meshlet_datas;

	// first calc total size, only consider unique meshes
	for (const auto &mesh_ptr : meshes_)
	{
		Mesh *mesh = mesh_ptr.get();

		// if this mesh has been processed, skip
		if (mesh_to_offsets.find(mesh) != mesh_to_offsets.end())
		{
			continue;
		}

		mesh_to_offsets[mesh] = {global_vertices_count_, global_indices_count_};
		total_vertex_size += uint32_t(mesh->vertices_count) * sizeof(MeshData::Vertex);
		total_index_size += uint32_t(mesh->indices_count) * sizeof(MeshData::IndexType);

		if (build_meshlet)
		{
			mesh->mesh_data.append_meshlets(all_meshlets, all_meshlet_datas, global_vertices_count_);
		}

		all_mesh_infos[mesh->global_mesh_index] = {mesh->mesh_data.sphere_bound, global_vertices_count_, global_indices_count_, uint32_t(mesh->indices_count)};

		global_vertices_count_ += uint32_t(mesh->vertices_count);
		global_indices_count_ += uint32_t(mesh->indices_count);
	}

	// create temp buffer to collect all data
	std::vector<MeshData::Vertex>    all_vertices;
	std::vector<MeshData::IndexType> all_indices;
	all_vertices.reserve(global_vertices_count_);
	all_indices.reserve(global_indices_count_);

	if (build_meshlet)
	{
		while (all_meshlets.size() % 32)
		{
			all_meshlets.push_back(Meshlet());
		}
		global_meshlet_count_ = uint32_t(all_meshlets.size());
	}

	// record current offset
	uint32_t current_vertex_offset = 0;
	uint32_t current_index_offset  = 0;

	for (const auto &mesh_ptr : meshes_)
	{
		Mesh *mesh = mesh_ptr.get();
		// get vertex data
		all_vertices.insert(all_vertices.end(), mesh->mesh_data.vertices.begin(), mesh->mesh_data.vertices.end());
		all_indices.insert(all_indices.end(), mesh->mesh_data.indices.begin(), mesh->mesh_data.indices.end());
	}

	std::vector<MeshDraw> all_mesh_draws;
	all_mesh_draws.resize(objects_.size());
	//
	for (size_t index = 0; index < objects_.size(); ++index)
	{
		auto &object                = objects_[index];
		auto  offsets               = mesh_to_offsets[object.mesh];
		object.global_vertex_offset = offsets.first;
		object.global_index_offset  = offsets.second;

		all_mesh_draws[index].mesh_index   = object.mesh->global_mesh_index;
		all_mesh_draws[index].model_matrix = object.obj_to_world;
	}

	lz::ExecuteOnceQueue transfer_queue(core_);
	auto                 transfer_command_buffer = transfer_queue.begin_command_buffer();

	auto physical_device  = core_->get_physical_device();
	auto logical_device   = core_->get_logical_device();
	global_vertex_buffer_ = std::make_unique<lz::StagedBuffer>(physical_device, logical_device, total_vertex_size, vk::BufferUsageFlagBits::eStorageBuffer);
	memcpy(global_vertex_buffer_->map(), all_vertices.data(), total_vertex_size);
	global_vertex_buffer_->unmap(transfer_command_buffer);

	if (total_index_size > 0)
	{
		global_index_buffer_ = std::make_unique<lz::StagedBuffer>(physical_device, logical_device, total_index_size, vk::BufferUsageFlagBits::eIndexBuffer);
		memcpy(global_index_buffer_->map(), all_indices.data(), total_index_size);
		global_index_buffer_->unmap(transfer_command_buffer);
	}

	// Mesh info and mesh draw buffer
	global_mesh_info_buffer_ = std::make_unique<lz::StagedBuffer>(physical_device, logical_device, all_mesh_infos.size() * sizeof(MeshInfo), vk::BufferUsageFlagBits::eStorageBuffer);
	memcpy(global_mesh_info_buffer_->map(), all_mesh_infos.data(), all_mesh_infos.size() * sizeof(MeshInfo));
	global_mesh_info_buffer_->unmap(transfer_command_buffer);
	global_mesh_draw_buffer_ = std::make_unique<lz::StagedBuffer>(physical_device, logical_device, all_mesh_draws.size() * sizeof(MeshDraw), vk::BufferUsageFlagBits::eStorageBuffer);
	memcpy(global_mesh_draw_buffer_->map(), all_mesh_draws.data(), all_mesh_draws.size() * sizeof(MeshDraw));
	global_mesh_draw_buffer_->unmap(transfer_command_buffer);

	if (build_meshlet && all_meshlets.size() > 0)
	{
		global_meshlet_buffer_ = std::make_unique<lz::StagedBuffer>(physical_device, logical_device, all_meshlets.size() * sizeof(Meshlet), vk::BufferUsageFlagBits::eStorageBuffer);
		memcpy(global_meshlet_buffer_->map(), all_meshlets.data(), all_meshlets.size() * sizeof(Meshlet));
		global_meshlet_buffer_->unmap(transfer_command_buffer);

		global_meshlet_data_buffer_ = std::make_unique<lz::StagedBuffer>(physical_device, logical_device, all_meshlet_datas.size() * sizeof(uint32_t), vk::BufferUsageFlagBits::eStorageBuffer);
		memcpy(global_meshlet_data_buffer_->map(), all_meshlet_datas.data(), all_meshlet_datas.size() * sizeof(uint32_t));
		global_meshlet_data_buffer_->unmap(transfer_command_buffer);
	}

	transfer_queue.end_command_buffer();
}

void Scene::create_draw_buffer()
{
	// Debug code
	LOGD("generating draw indirect buffer");
	std::vector<vk::DrawIndexedIndirectCommand> draw_commands(objects_.size());
	std::vector<glm::mat4>                      object_models(objects_.size());
	// Iterate through objects and record draw commands
	for (size_t index = 0; index < objects_.size(); ++index)
	{
		const auto &                   object = objects_[index];
		vk::DrawIndexedIndirectCommand draw_command;
		draw_command.firstInstance = 0;
		draw_command.firstIndex    = object.global_index_offset;
		draw_command.vertexOffset  = object.global_vertex_offset;
		draw_command.indexCount    = uint32_t(object.mesh->indices_count);
		draw_command.instanceCount = 1;
		draw_commands[index]       = std::move(draw_command);

		object_models[index] = object.obj_to_world;
	}

	// upload
	lz::ExecuteOnceQueue transfer_queue(core_);
	auto                 transfer_command_buffer = transfer_queue.begin_command_buffer();

	auto physical_device  = core_->get_physical_device();
	auto logical_device   = core_->get_logical_device();
	draw_indirect_buffer_ = std::make_unique<lz::StagedBuffer>(physical_device, logical_device,
	                                                           draw_commands.size() * sizeof(vk::DrawIndexedIndirectCommand),
	                                                           vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer);
	memcpy(draw_indirect_buffer_->map(), draw_commands.data(), draw_commands.size() * sizeof(vk::DrawIndexedIndirectCommand));
	draw_indirect_buffer_->unmap(transfer_command_buffer);

	draw_call_buffer_ = std::make_unique<lz::StagedBuffer>(physical_device, logical_device, object_models.size() * sizeof(glm::mat4), vk::BufferUsageFlagBits::eStorageBuffer);
	memcpy(draw_call_buffer_->map(), object_models.data(), object_models.size() * sizeof(glm::mat4));
	draw_call_buffer_->unmap(transfer_command_buffer);

	transfer_queue.end_command_buffer();
}

void Scene::create_draw_call_info_buffer()
{
	// Debug code
	LOGD("generating draw call info buffer");

	std::vector<MeshDraw> mesh_draws(objects_.size());
	for (size_t index = 0; index < objects_.size(); ++index)
	{
		const auto &object             = objects_[index];
		mesh_draws[index].mesh_index   = object.mesh->global_mesh_index;
		mesh_draws[index].model_matrix = object.obj_to_world;
	}

	lz::ExecuteOnceQueue transfer_queue(core_);
	auto                 transfer_command_buffer = transfer_queue.begin_command_buffer();

	auto physical_device = core_->get_physical_device();
	auto logical_device  = core_->get_logical_device();

	draw_call_info_buffer_ = std::make_unique<lz::StagedBuffer>(physical_device, logical_device, mesh_draws.size() * sizeof(MeshDraw), vk::BufferUsageFlagBits::eStorageBuffer);
	memcpy(draw_call_info_buffer_->map(), mesh_draws.data(), mesh_draws.size() * sizeof(MeshDraw));
	draw_call_info_buffer_->unmap(transfer_command_buffer);

	transfer_queue.end_command_buffer();
}

Buffer &Scene::get_global_vertex_buffer() const
{
	return global_vertex_buffer_->get_buffer();
}

vk::Buffer Scene::get_global_index_buffer() const
{
	return global_index_buffer_ ? global_index_buffer_->get_buffer().get_handle() : nullptr;
}

Buffer &Scene::get_draw_call_buffer() const
{
	return draw_call_buffer_->get_buffer();
}

Buffer &Scene::get_draw_indirect_buffer() const
{
	return draw_indirect_buffer_->get_buffer();
}

Buffer &Scene::get_global_meshlet_buffer() const
{
	return global_meshlet_buffer_->get_buffer();
}

Buffer &Scene::get_global_meshlet_data_buffer() const
{
	return global_meshlet_data_buffer_->get_buffer();
}

Buffer &Scene::get_draw_call_info_buffer() const
{
	return draw_call_info_buffer_->get_buffer();
}

Buffer &Scene::get_global_mesh_info_buffer() const
{
	return global_mesh_info_buffer_->get_buffer();
}	

Buffer &Scene::get_global_mesh_draw_buffer() const
{
	return global_mesh_draw_buffer_->get_buffer();
}


}        // namespace lz
