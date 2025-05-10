#include "Scene.h"

#include "backend/Core.h"
#include "backend/Logging.h"
#include "backend/PresentQueue.h"
#include "tiny_gltf.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

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

// New constructor for GLTF file loading
JsonScene::JsonScene(const std::string &file_path, lz::Core *core, GeometryTypes geometry_type)
{
	this->core_  = core;
	vertex_decl_ = Mesh::get_vertex_declaration();

	// Check file extension to determine loading method
	std::filesystem::path path(file_path);
	std::string           ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	if (ext == ".gltf" || ext == ".glb")
	{
		// Load from GLTF
		load_from_gltf(file_path);
	}
	else
	{
		// Unsupported file format
		LOGE("Unsupported file format: {}", ext);
		throw std::runtime_error("Unsupported file format");
	}
}

// Helper method to load meshes and create objects from a GLTF file
void JsonScene::load_from_gltf(const std::string &file_path)
{
	LOGI("Loading GLTF scene: {}", file_path);

	tinygltf::Model    model;
	tinygltf::TinyGLTF loader;
	std::string        err;
	std::string        warn;

	// Detect if we need to load binary or ASCII glTF
	bool                  ret = false;
	std::filesystem::path path(file_path);
	if (path.extension() == ".glb")
	{
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, file_path);
	}
	else
	{
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, file_path);
	}

	if (!warn.empty())
	{
		LOGW("Warning loading GLTF: {}", warn);
	}

	if (!err.empty())
	{
		LOGE("Error loading GLTF: {}", err);
	}

	if (!ret)
	{
		throw std::runtime_error("Failed to load GLTF model");
	}

	// Map to store mesh pointers by their index in the GLTF file
	std::map<int, Mesh *> mesh_map;

	// Create meshes from GLTF model
	for (size_t i = 0; i < model.meshes.size(); i++)
	{
		const tinygltf::Mesh &gltf_mesh = model.meshes[i];

		for (size_t j = 0; j < gltf_mesh.primitives.size(); j++)
		{
			// For each primitive, create a mesh
			const tinygltf::Primitive &primitive = gltf_mesh.primitives[j];

			// Skip primitives without position
			if (primitive.attributes.find("POSITION") == primitive.attributes.end())
			{
				continue;
			}

			// Create a new mesh
			auto mesh = std::make_unique<Mesh>(MeshData());

			// Extract positions
			const tinygltf::Accessor   &pos_accessor = model.accessors[primitive.attributes.at("POSITION")];
			const tinygltf::BufferView &pos_view     = model.bufferViews[pos_accessor.bufferView];
			const tinygltf::Buffer     &pos_buffer   = model.buffers[pos_view.buffer];

			// Get vertices count
			size_t vertices_count = pos_accessor.count;
			mesh->vertices_count  = vertices_count;

			// Prepare vertices data
			std::vector<lz::Vertex> vertices(vertices_count);

			// Get position data
			const float *pos_data = reinterpret_cast<const float *>(&pos_buffer.data[pos_view.byteOffset + pos_accessor.byteOffset]);
			for (size_t v = 0; v < vertices_count; v++)
			{
				vertices[v].pos.x = pos_data[v * 3 + 0];
				vertices[v].pos.y = pos_data[v * 3 + 1];
				vertices[v].pos.z = pos_data[v * 3 + 2];

				// Default values for other attributes
				vertices[v].normal = glm::vec3(0.0f, 1.0f, 0.0f);
				vertices[v].uv     = glm::vec2(0.0f);
			}

			// Check for normal attribute
			if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
			{
				const tinygltf::Accessor   &normal_accessor = model.accessors[primitive.attributes.at("NORMAL")];
				const tinygltf::BufferView &normal_view     = model.bufferViews[normal_accessor.bufferView];
				const tinygltf::Buffer     &normal_buffer   = model.buffers[normal_view.buffer];

				const float *normal_data = reinterpret_cast<const float *>(&normal_buffer.data[normal_view.byteOffset + normal_accessor.byteOffset]);
				for (size_t v = 0; v < vertices_count; v++)
				{
					vertices[v].normal.x = normal_data[v * 3 + 0];
					vertices[v].normal.y = normal_data[v * 3 + 1];
					vertices[v].normal.z = normal_data[v * 3 + 2];
				}
			}

			// Check for texcoord attribute
			if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
			{
				const tinygltf::Accessor   &texcoord_accessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
				const tinygltf::BufferView &texcoord_view     = model.bufferViews[texcoord_accessor.bufferView];
				const tinygltf::Buffer     &texcoord_buffer   = model.buffers[texcoord_view.buffer];

				const float *texcoord_data = reinterpret_cast<const float *>(&texcoord_buffer.data[texcoord_view.byteOffset + texcoord_accessor.byteOffset]);
				for (size_t v = 0; v < vertices_count; v++)
				{
					vertices[v].uv.x = texcoord_data[v * 2 + 0];
					vertices[v].uv.y = texcoord_data[v * 2 + 1];
				}
			}

			// Check for color attribute - Vertex has no color field, we'll use normal to store colors
			if (primitive.attributes.find("COLOR_0") != primitive.attributes.end())
			{
				const tinygltf::Accessor   &color_accessor = model.accessors[primitive.attributes.at("COLOR_0")];
				const tinygltf::BufferView &color_view     = model.bufferViews[color_accessor.bufferView];
				const tinygltf::Buffer     &color_buffer   = model.buffers[color_view.buffer];

				// Handle different color component types and sizes
				if (color_accessor.type == TINYGLTF_TYPE_VEC3)
				{
					const float *color_data = reinterpret_cast<const float *>(&color_buffer.data[color_view.byteOffset + color_accessor.byteOffset]);
					for (size_t v = 0; v < vertices_count; v++)
					{
						vertices[v].normal.r = color_data[v * 3 + 0];
						vertices[v].normal.g = color_data[v * 3 + 1];
						vertices[v].normal.b = color_data[v * 3 + 2];
					}
				}
				else if (color_accessor.type == TINYGLTF_TYPE_VEC4)
				{
					const float *color_data = reinterpret_cast<const float *>(&color_buffer.data[color_view.byteOffset + color_accessor.byteOffset]);
					for (size_t v = 0; v < vertices_count; v++)
					{
						vertices[v].normal.r = color_data[v * 4 + 0];
						vertices[v].normal.g = color_data[v * 4 + 1];
						vertices[v].normal.b = color_data[v * 4 + 2];
					}
				}
			}

			// Get indices
			std::vector<uint32_t> indices;
			if (primitive.indices >= 0)
			{
				const tinygltf::Accessor   &index_accessor = model.accessors[primitive.indices];
				const tinygltf::BufferView &index_view     = model.bufferViews[index_accessor.bufferView];
				const tinygltf::Buffer     &index_buffer   = model.buffers[index_view.buffer];

				size_t index_count = index_accessor.count;
				indices.resize(index_count);

				// Handle different index component types
				if (index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
				{
					const uint16_t *index_data = reinterpret_cast<const uint16_t *>(&index_buffer.data[index_view.byteOffset + index_accessor.byteOffset]);
					for (size_t idx = 0; idx < index_count; idx++)
					{
						indices[idx] = static_cast<uint32_t>(index_data[idx]);
					}
				}
				else if (index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
				{
					const uint32_t *index_data = reinterpret_cast<const uint32_t *>(&index_buffer.data[index_view.byteOffset + index_accessor.byteOffset]);
					for (size_t idx = 0; idx < index_count; idx++)
					{
						indices[idx] = index_data[idx];
					}
				}
				else if (index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
				{
					const uint8_t *index_data = reinterpret_cast<const uint8_t *>(&index_buffer.data[index_view.byteOffset + index_accessor.byteOffset]);
					for (size_t idx = 0; idx < index_count; idx++)
					{
						indices[idx] = static_cast<uint32_t>(index_data[idx]);
					}
				}

				mesh->indices_count = indices.size();
			}
			else
			{
				// No indices, create them
				indices.resize(vertices_count);
				for (size_t idx = 0; idx < vertices_count; idx++)
				{
					indices[idx] = static_cast<uint32_t>(idx);
				}
				mesh->indices_count = indices.size();
			}

			// Store vertices and indices in mesh data
			mesh->mesh_data.vertices = std::move(vertices);
			mesh->mesh_data.indices  = std::move(indices);

			mesh->primitive_topology = vk::PrimitiveTopology::eTriangleList;

			// Add mesh to meshes and map
			int mesh_idx            = static_cast<int>(meshes_.size());
			mesh_map[mesh_idx]      = mesh.get();
			mesh->global_mesh_index = uint32_t(meshes_.size());
			meshes_.push_back(std::move(mesh));
		}
	}

	// Process nodes to create objects
	// First, find the default scene
	int default_scene_idx = model.defaultScene >= 0 ? model.defaultScene : 0;

	if (model.scenes.size() > 0)
	{
		const tinygltf::Scene &scene = model.scenes[default_scene_idx];

		// Process each node in the scene
		for (int node_idx : scene.nodes)
		{
			// Process node recursively
			process_node(model, node_idx, mesh_map, glm::mat4(1.0f));
		}
	}

	// If no objects created (empty scene), log warning
	if (objects_.empty())
	{
		LOGW("No objects created from GLTF scene");
	}

	// Create global buffers
	create_global_buffers();
}

// Helper to process GLTF nodes recursively
void JsonScene::process_node(const tinygltf::Model &model, int node_idx,
                             const std::map<int, Mesh *> &mesh_map,
                             const glm::mat4             &parent_transform)
{
	const tinygltf::Node &node = model.nodes[node_idx];

	// Calculate node's transform
	glm::mat4 local_transform = glm::mat4(1.0f);

	// Apply translation
	if (node.translation.size() == 3)
	{
		local_transform = glm::translate(local_transform,
		                                 glm::vec3(node.translation[0], node.translation[1], node.translation[2]));
	}

	// Apply rotation (quaternion)
	if (node.rotation.size() == 4)
	{
		glm::quat q = glm::quat(
		    static_cast<float>(node.rotation[3]),        // w
		    static_cast<float>(node.rotation[0]),        // x
		    static_cast<float>(node.rotation[1]),        // y
		    static_cast<float>(node.rotation[2])         // z
		);
		// 使用glm::mat4_cast将四元数转换为矩阵并与local_transform相乘
		local_transform = local_transform * glm::mat4_cast(q);
	}

	// Apply scale
	if (node.scale.size() == 3)
	{
		local_transform = glm::scale(local_transform,
		                             glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
	}

	// Apply matrix directly if specified
	if (node.matrix.size() == 16)
	{
		glm::mat4 matrix;
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				matrix[i][j] = static_cast<float>(node.matrix[i * 4 + j]);
			}
		}
		local_transform = matrix;
	}

	// Combine with parent transform
	glm::mat4 node_transform = parent_transform * local_transform;

	// If node has a mesh, create an object
	if (node.mesh >= 0 && mesh_map.find(node.mesh) != mesh_map.end())
	{
		Object object;
		object.mesh               = mesh_map.at(node.mesh);
		object.obj_to_world       = node_transform;
		object.albedo_color       = glm::vec3(1.0f);        // Default color
		object.emissive_color     = glm::vec3(0.0f);        // No emission by default
		object.is_shadow_receiver = true;

		objects_.push_back(object);
	}

	// Process children recursively
	for (int child : node.children)
	{
		process_node(model, child, mesh_map, node_transform);
	}
}

JsonScene::JsonScene(Json::Value scene_config, lz::Core *core, GeometryTypes geometry_type)
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

			auto mesh               = std::unique_ptr<Mesh>(new Mesh(mesh_data));
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

// void JsonScene::iterate_objects(ObjectCallback object_callback)
//{
//	for (auto &object : objects_)
//	{
//		object_callback(object.obj_to_world, object.albedo_color, object.emissive_color,
//		                object.mesh->vertex_buffer->get_buffer().get_handle(),
//		                object.mesh->index_buffer ? object.mesh->index_buffer->get_buffer().get_handle() : nullptr,
//		                uint32_t(object.mesh->vertices_count), uint32_t(object.mesh->indices_count));
//	}
// }

void JsonScene::create_global_buffers(bool build_meshlet)
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
		total_vertex_size += uint32_t(mesh->vertices_count) * sizeof(lz::Vertex);
		total_index_size += uint32_t(mesh->indices_count) * sizeof(MeshData::IndexType);

		if (build_meshlet)
		{
			mesh->mesh_data.append_meshlets(all_meshlets, all_meshlet_datas, global_vertices_count_);
		}

		// 使用static_cast显式转换size_t到uint32_t，避免编译警告
		all_mesh_infos[mesh->global_mesh_index] = {
		    mesh->mesh_data.sphere_bound,
		    static_cast<uint32_t>(global_vertices_count_),
		    static_cast<uint32_t>(global_indices_count_),
		    static_cast<uint32_t>(mesh->indices_count)};

		global_vertices_count_ += uint32_t(mesh->vertices_count);
		global_indices_count_ += uint32_t(mesh->indices_count);
	}

	// create temp buffer to collect all data
	std::vector<lz::Vertex>          all_vertices;
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

void JsonScene::create_draw_buffer()
{
	// Debug code
	LOGD("generating draw indirect buffer");
	std::vector<vk::DrawIndexedIndirectCommand> draw_commands(objects_.size());
	std::vector<glm::mat4>                      object_models(objects_.size());
	// Iterate through objects and record draw commands
	for (size_t index = 0; index < objects_.size(); ++index)
	{
		const auto                    &object = objects_[index];
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

void JsonScene::create_draw_call_info_buffer()
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

Buffer &JsonScene::get_global_vertex_buffer() const
{
	return global_vertex_buffer_->get_buffer();
}

vk::Buffer JsonScene::get_global_index_buffer() const
{
	return global_index_buffer_ ? global_index_buffer_->get_buffer().get_handle() : nullptr;
}

Buffer &JsonScene::get_draw_call_buffer() const
{
	return draw_call_buffer_->get_buffer();
}

Buffer &JsonScene::get_draw_indirect_buffer() const
{
	return draw_indirect_buffer_->get_buffer();
}

Buffer &JsonScene::get_global_meshlet_buffer() const
{
	return global_meshlet_buffer_->get_buffer();
}

Buffer &JsonScene::get_global_meshlet_data_buffer() const
{
	return global_meshlet_data_buffer_->get_buffer();
}

Buffer &JsonScene::get_draw_call_info_buffer() const
{
	return draw_call_info_buffer_->get_buffer();
}

Buffer &JsonScene::get_global_mesh_info_buffer() const
{
	return global_mesh_info_buffer_->get_buffer();
}

Buffer &JsonScene::get_global_mesh_draw_buffer() const
{
	return global_mesh_draw_buffer_->get_buffer();
}

}        // namespace lz
