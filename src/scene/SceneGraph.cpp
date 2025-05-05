#include "SceneGraph.h"
#include "backend/Core.h"
#include "backend/Logging.h"
#include "backend/PresentQueue.h"
#include "filesystem"
#include "meshoptimizer.h"
#include "tiny_gltf.h"

#include "glm/ext/quaternion_common.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"

namespace lz
{
SceneGraph::SceneGraph(lz::Core *core) :
    core_(core)
{
}

SceneGraph::~SceneGraph()
{
	clear();
}

void SceneGraph::clear()
{
	meshes_.clear();
	nodes_.clear();
	submeshes_.clear();
	draw_commands_.clear();
	name_to_node_.clear();

	global_vertex_buffer_.reset();
	global_index_buffer_.reset();
	draw_indirect_buffer_.reset();

	global_vertices_count_ = 0;
	global_indices_count_  = 0;
	buffers_created_       = false;
	commands_dirty_        = true;
}

size_t SceneGraph::load_model(const std::string &filepath, const glm::vec3 &scale)
{
	LOGI("Loading model: {}", filepath);

	tinygltf::Model    model;
	tinygltf::TinyGLTF loader;
	std::string        err;
	std::string        warn;

	bool                  ret = false;
	std::filesystem::path path(filepath);
	if (path.extension() == ".glb")
	{
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, filepath);
	}
	else
	{
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, filepath);
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

	// 保存原始节点数用于根节点引用
	size_t original_node_count = nodes_.size();
	size_t mesh_start_index    = meshes_.size();

	// 创建一个传输命令用于加载网格
	lz::ExecuteOnceQueue transfer_queue(core_);
	auto                 transfer_command_buffer = transfer_queue.begin_command_buffer();

	// 加载模型中的所有网格
	for (size_t i = 0; i < model.meshes.size(); i++)
	{
		const tinygltf::Mesh &gltf_mesh = model.meshes[i];

		for (size_t j = 0; j < gltf_mesh.primitives.size(); j++)
		{
			const tinygltf::Primitive &primitive = gltf_mesh.primitives[j];

			// Skip primitives without position
			if (primitive.attributes.find("POSITION") == primitive.attributes.end())
			{
				continue;
			}

			// 创建Mesh对象，但不创建独立的VkBuffer
			auto mesh = std::make_unique<Mesh>(
			    MeshData(),
			    core_->get_physical_device(),
			    core_->get_logical_device(),
			    transfer_command_buffer);

			// Extract positions
			const tinygltf::Accessor &  pos_accessor = model.accessors[primitive.attributes.at("POSITION")];
			const tinygltf::BufferView &pos_view     = model.bufferViews[pos_accessor.bufferView];
			const tinygltf::Buffer &    pos_buffer   = model.buffers[pos_view.buffer];

			// Optional normal and texcoord accessors
			const tinygltf::Accessor *  normal_accessor = nullptr;
			const tinygltf::BufferView *normal_view     = nullptr;
			const tinygltf::Buffer *    normal_buffer   = nullptr;

			if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
			{
				normal_accessor = &model.accessors[primitive.attributes.at("NORMAL")];
				normal_view     = &model.bufferViews[normal_accessor->bufferView];
				normal_buffer   = &model.buffers[normal_view->buffer];
			}

			const tinygltf::Accessor *  texcoord_accessor = nullptr;
			const tinygltf::BufferView *texcoord_view     = nullptr;
			const tinygltf::Buffer *    texcoord_buffer   = nullptr;

			if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
			{
				texcoord_accessor = &model.accessors[primitive.attributes.at("TEXCOORD_0")];
				texcoord_view     = &model.bufferViews[texcoord_accessor->bufferView];
				texcoord_buffer   = &model.buffers[texcoord_view->buffer];
			}

			// Calculate bounds
			glm::vec3 min_bound(std::numeric_limits<float>::max());
			glm::vec3 max_bound(std::numeric_limits<float>::lowest());

			// Create vertices
			std::vector<lz::Vertex> vertices;
			vertices.resize(pos_accessor.count);

			// Extract positions
			const float *pos_data   = reinterpret_cast<const float *>(&pos_buffer.data[pos_view.byteOffset + pos_accessor.byteOffset]);
			const size_t pos_stride = pos_view.byteStride ? pos_view.byteStride / sizeof(float) : 3;

			for (size_t v = 0; v < pos_accessor.count; v++)
			{
				vertices[v].pos.x = pos_data[v * pos_stride + 0] * scale.x;
				vertices[v].pos.y = pos_data[v * pos_stride + 1] * scale.y;
				vertices[v].pos.z = pos_data[v * pos_stride + 2] * scale.z;

				min_bound = glm::min(min_bound, vertices[v].pos);
				max_bound = glm::max(max_bound, vertices[v].pos);
			}

			// Extract normals if present
			if (normal_accessor)
			{
				const float *normal_data   = reinterpret_cast<const float *>(&normal_buffer->data[normal_view->byteOffset + normal_accessor->byteOffset]);
				const size_t normal_stride = normal_view->byteStride ? normal_view->byteStride / sizeof(float) : 3;

				for (size_t v = 0; v < normal_accessor->count; v++)
				{
					vertices[v].normal.x = normal_data[v * normal_stride + 0];
					vertices[v].normal.y = normal_data[v * normal_stride + 1];
					vertices[v].normal.z = normal_data[v * normal_stride + 2];
				}
			}
			else
			{
				// Default normals
				for (size_t v = 0; v < vertices.size(); v++)
				{
					vertices[v].normal = glm::vec3(0.0f, 1.0f, 0.0f);
				}
			}

			// Extract texture coordinates if present
			if (texcoord_accessor)
			{
				const float *texcoord_data   = reinterpret_cast<const float *>(&texcoord_buffer->data[texcoord_view->byteOffset + texcoord_accessor->byteOffset]);
				const size_t texcoord_stride = texcoord_view->byteStride ? texcoord_view->byteStride / sizeof(float) : 2;

				for (size_t v = 0; v < texcoord_accessor->count; v++)
				{
					vertices[v].uv.x = texcoord_data[v * texcoord_stride + 0];
					vertices[v].uv.y = texcoord_data[v * texcoord_stride + 1];
				}
			}
			else
			{
				// Default UVs
				for (size_t v = 0; v < vertices.size(); v++)
				{
					vertices[v].uv = glm::vec2(0.0f, 0.0f);
				}
			}

			// Extract indices
			std::vector<uint32_t> indices;
			if (primitive.indices >= 0)
			{
				const tinygltf::Accessor &  index_accessor = model.accessors[primitive.indices];
				const tinygltf::BufferView &index_view     = model.bufferViews[index_accessor.bufferView];
				const tinygltf::Buffer &    index_buffer   = model.buffers[index_view.buffer];

				indices.resize(index_accessor.count);

				const unsigned char *data = &index_buffer.data[index_view.byteOffset + index_accessor.byteOffset];
				switch (index_accessor.componentType)
				{
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
					{
						for (size_t idx = 0; idx < index_accessor.count; idx++)
						{
							indices[idx] = static_cast<uint32_t>(data[idx]);
						}
						break;
					}
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
					{
						const uint16_t *short_data = reinterpret_cast<const uint16_t *>(data);
						for (size_t idx = 0; idx < index_accessor.count; idx++)
						{
							indices[idx] = static_cast<uint32_t>(short_data[idx]);
						}
						break;
					}
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
					{
						const uint32_t *int_data = reinterpret_cast<const uint32_t *>(data);
						for (size_t idx = 0; idx < index_accessor.count; idx++)
						{
							indices[idx] = int_data[idx];
						}
						break;
					}
					default:
						throw std::runtime_error("Unsupported index component type");
				}
			}
			else
			{
				// If no indices, create sequential indices
				indices.resize(vertices.size());
				for (size_t idx = 0; idx < vertices.size(); idx++)
				{
					indices[idx] = static_cast<uint32_t>(idx);
				}
			}

			// Optimize the mesh
			size_t                index_count = indices.size();
			std::vector<uint32_t> remap(index_count);
			size_t                vertex_count = meshopt_generateVertexRemap(remap.data(), indices.data(), index_count, vertices.data(), vertices.size(), sizeof(lz::Vertex));

			std::vector<lz::Vertex> tmp_vertices(vertex_count);
			std::vector<uint32_t>   tmp_indices(index_count);

			meshopt_remapVertexBuffer(tmp_vertices.data(), vertices.data(), vertices.size(), sizeof(lz::Vertex), remap.data());
			meshopt_remapIndexBuffer(tmp_indices.data(), indices.data(), index_count, remap.data());

			meshopt_optimizeVertexCache(tmp_indices.data(), tmp_indices.data(), index_count, vertex_count);
			meshopt_optimizeVertexFetch(tmp_vertices.data(), tmp_indices.data(), index_count, tmp_vertices.data(), vertex_count, sizeof(lz::Vertex));

			// 在完成网格处理后初始化indices_count和vertices_count
			mesh->mesh_data.vertices = std::move(tmp_vertices);
			mesh->mesh_data.indices  = std::move(tmp_indices);
			mesh->vertices_count     = mesh->mesh_data.vertices.size();
			mesh->indices_count      = mesh->mesh_data.indices.size();

			// 设置边界球
			mesh->mesh_data.sphere_bound = glm::vec4(0.5f * (min_bound + max_bound), glm::length(max_bound - min_bound) * 0.5f);

			// 添加对应的SubMesh
			SubMesh submesh;
			submesh.mesh_index   = meshes_.size();
			submesh.index_offset = 0;        // 将在创建全局缓冲区时调整
			submesh.index_count  = mesh->indices_count;

			// Set material properties if available
			if (primitive.material >= 0)
			{
				const tinygltf::Material &mat = model.materials[primitive.material];

				// Base color factor
				if (mat.pbrMetallicRoughness.baseColorFactor.size() >= 3)
				{
					submesh.albedo_color = glm::vec3(
					    static_cast<float>(mat.pbrMetallicRoughness.baseColorFactor[0]),
					    static_cast<float>(mat.pbrMetallicRoughness.baseColorFactor[1]),
					    static_cast<float>(mat.pbrMetallicRoughness.baseColorFactor[2]));
				}

				// Emissive factor
				if (mat.emissiveFactor.size() >= 3)
				{
					submesh.emissive_color = glm::vec3(
					    static_cast<float>(mat.emissiveFactor[0]),
					    static_cast<float>(mat.emissiveFactor[1]),
					    static_cast<float>(mat.emissiveFactor[2]));
				}
			}

			submeshes_.push_back(submesh);
			meshes_.push_back(std::move(mesh));
		}
	}

	// 完成传输命令
	transfer_queue.end_command_buffer();

	// Load nodes
	for (size_t i = 0; i < model.nodes.size(); i++)
	{
		const tinygltf::Node &gltf_node = model.nodes[i];

		Node node;
		node.name = gltf_node.name;

		// Set transform
		if (gltf_node.matrix.size() == 16)
		{
			// Direct matrix
			glm::mat4 matrix;
			for (int col = 0; col < 4; col++)
			{
				for (int row = 0; row < 4; row++)
				{
					matrix[col][row] = static_cast<float>(gltf_node.matrix[col * 4 + row]);
				}
			}
			node.local_transform = matrix;
		}
		else
		{
			// TRS (Translation, Rotation, Scale)
			glm::mat4 translation_mat(1.0f);
			if (gltf_node.translation.size() == 3)
			{
				translation_mat = glm::translate(glm::vec3(
				    static_cast<float>(gltf_node.translation[0]),
				    static_cast<float>(gltf_node.translation[1]),
				    static_cast<float>(gltf_node.translation[2])));
			}

			glm::mat4 rotation_mat(1.0f);
			if (gltf_node.rotation.size() == 4)
			{
				glm::quat q(
				    static_cast<float>(gltf_node.rotation[3]),        // w
				    static_cast<float>(gltf_node.rotation[0]),        // x
				    static_cast<float>(gltf_node.rotation[1]),        // y
				    static_cast<float>(gltf_node.rotation[2])         // z
				);
				rotation_mat = glm::mat4_cast(q);
			}

			glm::mat4 scale_mat(1.0f);
			if (gltf_node.scale.size() == 3)
			{
				scale_mat = glm::scale(glm::vec3(
				    static_cast<float>(gltf_node.scale[0]),
				    static_cast<float>(gltf_node.scale[1]),
				    static_cast<float>(gltf_node.scale[2])));
			}

			// Combine TRS
			node.local_transform = translation_mat * rotation_mat * scale_mat;
		}

		// Assign mesh to node if it has one
		if (gltf_node.mesh >= 0)
		{
			// Find all submeshes associated with this mesh
			const tinygltf::Mesh &gltf_mesh = model.meshes[gltf_node.mesh];
			for (size_t j = 0; j < gltf_mesh.primitives.size(); j++)
			{
				size_t submesh_index = mesh_start_index + j;
				if (submesh_index < submeshes_.size())
				{
					node.submesh_indices.push_back(submesh_index);
				}
			}
		}

		// Add children
		for (size_t child_index : gltf_node.children)
		{
			node.children.push_back(child_index + original_node_count);
		}

		// Add node to the scene graph
		size_t node_index = nodes_.size();
		if (!node.name.empty())
		{
			name_to_node_[node.name] = node_index;
		}
		nodes_.push_back(node);
	}

	// Set parent references for all nodes
	for (size_t i = 0; i < model.nodes.size(); i++)
	{
		size_t node_index = original_node_count + i;
		for (size_t child_index : nodes_[node_index].children)
		{
			nodes_[child_index].parent = node_index;
		}
	}

	// Find the root node(s) from the scene
	size_t root_node_index = original_node_count;
	if (!model.scenes.empty() && model.defaultScene >= 0)
	{
		const tinygltf::Scene &scene = model.scenes[model.defaultScene];

		// If the scene has nodes, use the first one as root
		if (!scene.nodes.empty())
		{
			root_node_index = original_node_count + scene.nodes[0];
		}
	}

	// Mark buffers and commands as needing updates
	buffers_created_ = false;
	commands_dirty_  = true;

	// Update transforms to populate global transforms
	update_transforms();

	return root_node_index;
}

void SceneGraph::update_transforms()
{
	// Reset all global transforms
	for (auto &node : nodes_)
	{
		node.global_transform = node.local_transform;
	}

	// Find all root nodes (nodes without parents)
	for (size_t i = 0; i < nodes_.size(); i++)
	{
		if (nodes_[i].parent == std::numeric_limits<size_t>::max())
		{
			// This is a root node
			update_node_transform(i, glm::mat4(1.0f));
		}
	}

	// Mark commands as needing updates
	commands_dirty_ = true;
}

void SceneGraph::update_node_transform(size_t node_index, const glm::mat4 &parent_transform)
{
	Node &node = nodes_[node_index];

	// Calculate global transform
	node.global_transform = parent_transform * node.local_transform;

	// Update all children
	for (size_t child_index : node.children)
	{
		update_node_transform(child_index, node.global_transform);
	}
}

void SceneGraph::create_global_buffers()
{
	if (buffers_created_)
	{
		return;
	}

	// 计算所需的全局缓冲区大小
	global_vertices_count_ = 0;
	global_indices_count_  = 0;

	// 首先计算唯一网格所需的总大小
	std::unordered_map<size_t, std::pair<uint32_t, uint32_t>> mesh_offsets;

	for (size_t i = 0; i < meshes_.size(); i++)
	{
		const auto &mesh = meshes_[i];

		// 跳过已处理过的网格
		if (mesh_offsets.find(i) != mesh_offsets.end())
		{
			continue;
		}

		// 记录此网格在全局缓冲区中的偏移量
		mesh_offsets[i] = {global_vertices_count_, global_indices_count_};

		// 递增总计数
		global_vertices_count_ += static_cast<uint32_t>(mesh->vertices_count);
		global_indices_count_ += static_cast<uint32_t>(mesh->indices_count);
	}

	// 创建临时缓冲区收集所有顶点和索引数据
	std::vector<lz::Vertex>          all_vertices;
	std::vector<MeshData::IndexType> all_indices;
	all_vertices.reserve(global_vertices_count_);
	all_indices.reserve(global_indices_count_);

	// 收集所有顶点和索引数据，并设置每个Mesh的全局偏移量
	for (size_t i = 0; i < meshes_.size(); i++)
	{
		auto &mesh = meshes_[i];

		// 跳过已处理过的网格
		if (mesh_offsets.find(i) == mesh_offsets.end())
		{
			continue;
		}

		// 获取此网格的全局偏移量
		auto [vertex_offset, index_offset] = mesh_offsets[i];

		// 设置Mesh的全局偏移量
		mesh->global_vertex_offset = vertex_offset;
		mesh->global_index_offset  = index_offset;

		// 添加顶点和索引数据
		all_vertices.insert(all_vertices.end(), mesh->mesh_data.vertices.begin(), mesh->mesh_data.vertices.end());
		all_indices.insert(all_indices.end(), mesh->mesh_data.indices.begin(), mesh->mesh_data.indices.end());
	}

	// 创建并填充全局缓冲区
	lz::ExecuteOnceQueue transfer_queue(core_);
	auto                 transfer_command_buffer = transfer_queue.begin_command_buffer();

	// 创建全局顶点缓冲区
	global_vertex_buffer_ = std::make_unique<lz::StagedBuffer>(
	    core_->get_physical_device(),
	    core_->get_logical_device(),
	    all_vertices.size() * sizeof(lz::Vertex),
	    vk::BufferUsageFlagBits::eVertexBuffer);

	// 上传顶点数据
	memcpy(global_vertex_buffer_->map(), all_vertices.data(), all_vertices.size() * sizeof(lz::Vertex));
	global_vertex_buffer_->unmap(transfer_command_buffer);

	// 创建全局索引缓冲区
	global_index_buffer_ = std::make_unique<lz::StagedBuffer>(
	    core_->get_physical_device(),
	    core_->get_logical_device(),
	    all_indices.size() * sizeof(MeshData::IndexType),
	    vk::BufferUsageFlagBits::eIndexBuffer);

	// 上传索引数据
	memcpy(global_index_buffer_->map(), all_indices.data(), all_indices.size() * sizeof(uint32_t));
	global_index_buffer_->unmap(transfer_command_buffer);

	// 完成传输
	transfer_queue.end_command_buffer();

	// 更新子网格偏移量，使用Mesh的全局偏移量
	for (auto &submesh : submeshes_)
	{
		auto &mesh           = meshes_[submesh.mesh_index];
		submesh.index_offset = mesh->global_index_offset + submesh.index_offset;
	}

	buffers_created_ = true;
	commands_dirty_  = true;
}

void SceneGraph::create_draw_commands()
{
	if (!buffers_created_)
	{
		create_global_buffers();
	}

	if (!commands_dirty_)
	{
		return;
	}

	// Generate draw commands
	generate_draw_commands();

	// Create indirect draw buffer
	if (draw_commands_.empty())
	{
		return;
	}

	// Prepare indirect draw commands
	struct VkDrawIndexedIndirectCommand
	{
		uint32_t indexCount;
		uint32_t instanceCount;
		uint32_t firstIndex;
		int32_t  vertexOffset;
		uint32_t firstInstance;
	};

	std::vector<VkDrawIndexedIndirectCommand> indirect_commands;
	indirect_commands.reserve(draw_commands_.size());

	for (const auto &cmd : draw_commands_)
	{
		const auto &submesh = submeshes_[cmd.submesh_index];

		VkDrawIndexedIndirectCommand indirect_cmd{};
		indirect_cmd.indexCount    = static_cast<uint32_t>(submesh.index_count);
		indirect_cmd.instanceCount = 1;
		indirect_cmd.firstIndex    = static_cast<uint32_t>(submesh.index_offset);
		indirect_cmd.vertexOffset  = static_cast<int32_t>(cmd.global_vertex_offset);
		indirect_cmd.firstInstance = 0;

		indirect_commands.push_back(indirect_cmd);
	}

	// Create indirect buffer
	lz::ExecuteOnceQueue transfer_queue(core_);
	auto                 transfer_command_buffer = transfer_queue.begin_command_buffer();

	draw_indirect_buffer_ = std::make_unique<lz::StagedBuffer>(
	    core_->get_physical_device(),
	    core_->get_logical_device(),
	    indirect_commands.size() * sizeof(VkDrawIndexedIndirectCommand),
	    vk::BufferUsageFlagBits::eIndirectBuffer);

	// Upload indirect command data
	memcpy(draw_indirect_buffer_->map(), indirect_commands.data(), indirect_commands.size() * sizeof(VkDrawIndexedIndirectCommand));
	draw_indirect_buffer_->unmap(transfer_command_buffer);

	transfer_queue.end_command_buffer();

	commands_dirty_ = false;
}

void SceneGraph::generate_draw_commands()
{
	draw_commands_.clear();

	// 处理所有带有网格的节点
	for (size_t node_index = 0; node_index < nodes_.size(); node_index++)
	{
		const Node &node = nodes_[node_index];

		// 跳过不可见节点
		if (!node.visible)
		{
			continue;
		}

		// 跳过没有子网格的节点
		if (node.submesh_indices.empty())
		{
			continue;
		}

		// 为每个子网格创建绘制命令
		for (size_t submesh_index : node.submesh_indices)
		{
			const SubMesh &submesh = submeshes_[submesh_index];
			const auto &   mesh    = meshes_[submesh.mesh_index];

			DrawCommand cmd{};
			cmd.node_index    = node_index;
			cmd.submesh_index = submesh_index;
			cmd.transform     = node.global_transform;

			// 使用Mesh的全局偏移量
			cmd.global_vertex_offset = mesh->global_vertex_offset;
			cmd.global_index_offset  = static_cast<uint32_t>(submesh.index_offset);

			draw_commands_.push_back(cmd);
		}
	}
}

const std::vector<DrawCommand> &SceneGraph::get_draw_commands() const
{
	return draw_commands_;
}

Node &SceneGraph::get_node(size_t index)
{
	return nodes_[index];
}

size_t SceneGraph::find_node(const std::string &name) const
{
	auto it = name_to_node_.find(name);
	if (it != name_to_node_.end())
	{
		return it->second;
	}
	return std::numeric_limits<size_t>::max();
}

void SceneGraph::set_node_visibility(size_t node_index, bool visible)
{
	if (node_index >= nodes_.size())
	{
		return;
	}

	nodes_[node_index].visible = visible;
	commands_dirty_            = true;
}

Buffer &SceneGraph::get_global_vertex_buffer() const
{
	return global_vertex_buffer_->get_buffer();
}

vk::Buffer SceneGraph::get_global_index_buffer() const
{
	return global_index_buffer_->get_buffer().get_handle();
}

Buffer &SceneGraph::get_draw_indirect_buffer() const
{
	return draw_indirect_buffer_->get_buffer();
}

}        // namespace lz