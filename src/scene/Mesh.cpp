#include "Mesh.h"
#include "backend/Logging.h"
#include "meshoptimizer.h"
#include "tiny_gltf.h"
#include "tiny_obj_loader.h"
#include <filesystem>
#define GLM_ENABLE_EXPERIMENTAL
#include <functional>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>

#include "Config.h"

namespace tinyobj
{
static bool operator<(const tinyobj::index_t &left, const tinyobj::index_t &right)
{
	return std::tie(left.vertex_index, left.normal_index, left.texcoord_index) < std::tie(right.vertex_index, right.normal_index, right.texcoord_index);
}
}        // namespace tinyobj

namespace lz
{
MeshData::MeshData() :
    primitive_topology(vk::PrimitiveTopology::eTriangleList)
{
}

MeshData::MeshData(const std::string file_name, glm::vec3 scale)
{
	auto loader = MeshLoader::get_loader(file_name);
	if (!loader)
	{
		LOGE("Failed to find an appropriate loader for file: {}", file_name);
		throw std::runtime_error("Failed to load mesh: no appropriate loader found");
	}

	*this = loader->load(file_name, scale);
}

// Static MeshLoader factory method
std::shared_ptr<MeshLoader> MeshLoader::get_loader(const std::string &file_name)
{
	static std::vector<std::shared_ptr<MeshLoader>> loaders = {
	    std::make_shared<ObjLoader>(),
	    std::make_shared<GltfLoader>()};

	for (auto &loader : loaders)
	{
		if (loader->can_load(file_name))
		{
			return loader;
		}
	}

	return nullptr;
}

// ObjLoader implementation
bool ObjLoader::can_load(const std::string &file_name)
{
	std::filesystem::path path(file_name);
	std::string           ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	return ext == ".obj";
}

MeshData ObjLoader::load(const std::string &file_name, glm::vec3 scale)
{
	MeshData mesh_data;
	mesh_data.primitive_topology = vk::PrimitiveTopology::eTriangleList;

	tinyobj::attrib_t                attrib;
	std::vector<tinyobj::shape_t>    shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;

	LOGI("Loading OBJ mesh: {}", file_name);
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file_name.c_str(), nullptr);

	if (!warn.empty())
	{
		LOGW("Warning: {}", warn);
	}
	if (!ret)
	{
		LOGE("Error: {}", err);
		throw std::runtime_error("Failed to load mesh");
	}

	// bounding sphere
	glm::vec3 min_bound = glm::vec3(std::numeric_limits<float>::infinity());
	glm::vec3 max_bound = glm::vec3(-std::numeric_limits<float>::infinity());

	std::vector<lz::Vertex> triangle_vertices;
	for (auto &shape : shapes)
	{
		for (auto &index : shape.mesh.indices)
		{
			lz::Vertex vertex;
			vertex.pos = glm::vec3(
			    attrib.vertices[3 * index.vertex_index + 0] * scale.x,
			    attrib.vertices[3 * index.vertex_index + 1] * scale.y,
			    attrib.vertices[3 * index.vertex_index + 2] * scale.z);

			min_bound = glm::min(min_bound, vertex.pos);
			max_bound = glm::max(max_bound, vertex.pos);

			if (index.normal_index != -1)
			{
				vertex.normal = glm::vec3(
				    attrib.normals[3 * index.normal_index + 0],
				    attrib.normals[3 * index.normal_index + 1],
				    attrib.normals[3 * index.normal_index + 2]);
			}
			else
			{
				vertex.normal = glm::vec3(1.0f, 0.0f, 0.0f);
			}

			if (index.texcoord_index != -1)
			{
				vertex.uv = glm::vec2(
				    attrib.texcoords[2 * index.texcoord_index + 0],
				    attrib.texcoords[2 * index.texcoord_index + 1]);
			}
			else
			{
				vertex.uv = glm::vec2(0.0f, 0.0f);
			}

			triangle_vertices.push_back(vertex);
		}
	}

	size_t index_count = triangle_vertices.size();
	// mesh optimizer
	std::vector<uint32_t> remap(index_count);
	size_t                vertex_count = meshopt_generateVertexRemap(remap.data(), 0, index_count, triangle_vertices.data(), index_count, sizeof(lz::Vertex));

	std::vector<lz::Vertex> tmp_vertices(vertex_count);
	std::vector<uint32_t>   tmp_indices(index_count);

	meshopt_remapVertexBuffer(tmp_vertices.data(), triangle_vertices.data(), index_count, sizeof(lz::Vertex), remap.data());
	meshopt_remapIndexBuffer(tmp_indices.data(), 0, index_count, remap.data());

	meshopt_optimizeVertexCache(tmp_indices.data(), tmp_indices.data(), index_count, vertex_count);
	meshopt_optimizeVertexFetch(tmp_vertices.data(), tmp_indices.data(), index_count, tmp_vertices.data(), vertex_count, sizeof(lz::Vertex));

	mesh_data.vertices = std::move(tmp_vertices);
	mesh_data.indices  = std::move(tmp_indices);

	mesh_data.sphere_bound = glm::vec4(0.5f * (min_bound + max_bound), glm::length(max_bound - min_bound) * 0.5f);

	return mesh_data;
}

// GltfLoader implementation
bool GltfLoader::can_load(const std::string &file_name)
{
	std::filesystem::path path(file_name);
	std::string           ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	return ext == ".gltf" || ext == ".glb";
}

MeshData GltfLoader::load(const std::string &file_name, glm::vec3 scale)
{
	MeshData mesh_data;
	mesh_data.primitive_topology = vk::PrimitiveTopology::eTriangleList;

	LOGI("Loading GLTF mesh: {}", file_name);

	tinygltf::Model    model;
	tinygltf::TinyGLTF loader;
	std::string        err;
	std::string        warn;

	// Detect if we need to load binary or ASCII glTF
	bool                  ret = false;
	std::filesystem::path path(file_name);
	if (path.extension() == ".glb")
	{
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, file_name);
	}
	else
	{
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, file_name);
	}

	if (!warn.empty())
	{
		LOGW("Warning: {}", warn);
	}

	if (!err.empty())
	{
		LOGE("Error: {}", err);
	}

	if (!ret)
	{
		throw std::runtime_error("Failed to load GLTF mesh");
	}

	int scene_index = model.defaultScene >= 0 ? model.defaultScene : 0;
	if (model.scenes.empty() || scene_index >= model.scenes.size())
	{
		throw std::runtime_error("GLTF file contains no valid scenes");
	}

	std::vector<lz::Vertex> all_vertices;
	std::vector<uint32_t>   all_indices;
	uint32_t                vertex_offset = 0;

	// bounding sphere
	glm::vec3 min_bound = glm::vec3(std::numeric_limits<float>::infinity());
	glm::vec3 max_bound = glm::vec3(-std::numeric_limits<float>::infinity());

	// Define process_node function type for recursive lambda
	std::function<void(int, const glm::mat4 &)> process_node;

	// Recursive function to process nodes and their children
	process_node = [&](int node_index, const glm::mat4 &parent_transform) {
		if (node_index < 0 || node_index >= model.nodes.size())
			return;

		const auto &node = model.nodes[node_index];

		// Calculate node transformation matrix
		glm::mat4 local_transform(1.0f);

		// Process matrix transformation
		if (node.matrix.size() == 16)
		{
			// Use matrix directly from glTF
			local_transform = glm::mat4(
			    node.matrix[0], node.matrix[1], node.matrix[2], node.matrix[3],
			    node.matrix[4], node.matrix[5], node.matrix[6], node.matrix[7],
			    node.matrix[8], node.matrix[9], node.matrix[10], node.matrix[11],
			    node.matrix[12], node.matrix[13], node.matrix[14], node.matrix[15]);
		}
		else
		{
			// Use TRS (Translation, Rotation, Scale)
			if (node.translation.size() == 3)
			{
				local_transform = glm::translate(local_transform, glm::vec3(
				                                                      node.translation[0], node.translation[1], node.translation[2]));
			}

			if (node.rotation.size() == 4)
			{
				glm::quat q(
				    static_cast<float>(node.rotation[3]),        // w
				    static_cast<float>(node.rotation[0]),        // x
				    static_cast<float>(node.rotation[1]),        // y
				    static_cast<float>(node.rotation[2])         // z
				);
				local_transform = local_transform * glm::mat4_cast(q);
			}

			if (node.scale.size() == 3)
			{
				local_transform = glm::scale(local_transform, glm::vec3(
				                                                  node.scale[0], node.scale[1], node.scale[2]));
			}
		}

		// Combine parent and local transformations
		glm::mat4 node_transform = parent_transform * local_transform;

		// Process this node's mesh
		if (node.mesh >= 0 && node.mesh < model.meshes.size())
		{
			const auto &mesh = model.meshes[node.mesh];

			// Process all primitives in the mesh
			for (const auto &primitive : mesh.primitives)
			{
				// Check if we have position data
				if (primitive.attributes.find("POSITION") == primitive.attributes.end())
				{
					continue;        // Skip primitives without position data
				}

				// Get accessors for vertex data
				const auto &pos_accessor = model.accessors[primitive.attributes.at("POSITION")];
				const auto &pos_view     = model.bufferViews[pos_accessor.bufferView];
				const auto &pos_buffer   = model.buffers[pos_view.buffer];

				// Optional normal and texcoord accessors
				const tinygltf::Accessor   *normal_accessor = nullptr;
				const tinygltf::BufferView *normal_view     = nullptr;
				const tinygltf::Buffer     *normal_buffer   = nullptr;

				if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
				{
					normal_accessor = &model.accessors[primitive.attributes.at("NORMAL")];
					normal_view     = &model.bufferViews[normal_accessor->bufferView];
					normal_buffer   = &model.buffers[normal_view->buffer];
				}

				const tinygltf::Accessor   *texcoord_accessor = nullptr;
				const tinygltf::BufferView *texcoord_view     = nullptr;
				const tinygltf::Buffer     *texcoord_buffer   = nullptr;

				if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
				{
					texcoord_accessor = &model.accessors[primitive.attributes.at("TEXCOORD_0")];
					texcoord_view     = &model.bufferViews[texcoord_accessor->bufferView];
					texcoord_buffer   = &model.buffers[texcoord_view->buffer];
				}

				// Extract vertices
				std::vector<lz::Vertex> vertices;
				vertices.resize(pos_accessor.count);

				const float *pos_data   = reinterpret_cast<const float *>(&pos_buffer.data[pos_view.byteOffset + pos_accessor.byteOffset]);
				const size_t pos_stride = pos_view.byteStride ? pos_view.byteStride / sizeof(float) : 3;

				for (size_t i = 0; i < pos_accessor.count; i++)
				{
					glm::vec3 pos(
					    pos_data[i * pos_stride + 0],
					    pos_data[i * pos_stride + 1],
					    pos_data[i * pos_stride + 2]);

					// Apply node transformation and global scale
					glm::vec4 transformed_pos = node_transform * glm::vec4(pos, 1.0f);
					vertices[i].pos           = glm::vec3(transformed_pos) * scale;

					min_bound = glm::min(min_bound, vertices[i].pos);
					max_bound = glm::max(max_bound, vertices[i].pos);
				}

				// Extract normals if present
				if (normal_accessor)
				{
					const float *normal_data   = reinterpret_cast<const float *>(&normal_buffer->data[normal_view->byteOffset + normal_accessor->byteOffset]);
					const size_t normal_stride = normal_view->byteStride ? normal_view->byteStride / sizeof(float) : 3;

					for (size_t i = 0; i < normal_accessor->count; i++)
					{
						glm::vec3 normal(
						    normal_data[i * normal_stride + 0],
						    normal_data[i * normal_stride + 1],
						    normal_data[i * normal_stride + 2]);

						// Apply only rotation part of transform to normals
						glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(node_transform)));
						vertices[i].normal      = glm::normalize(normal_matrix * normal);
					}
				}
				else
				{
					// Default normals
					for (size_t i = 0; i < vertices.size(); i++)
					{
						vertices[i].normal = glm::vec3(1.0f, 0.0f, 0.0f);
					}
				}

				// Extract texture coordinates if present
				if (texcoord_accessor)
				{
					const float *texcoord_data   = reinterpret_cast<const float *>(&texcoord_buffer->data[texcoord_view->byteOffset + texcoord_accessor->byteOffset]);
					const size_t texcoord_stride = texcoord_view->byteStride ? texcoord_view->byteStride / sizeof(float) : 2;

					for (size_t i = 0; i < texcoord_accessor->count; i++)
					{
						vertices[i].uv.x = texcoord_data[i * texcoord_stride + 0];
						vertices[i].uv.y = texcoord_data[i * texcoord_stride + 1];
					}
				}
				else
				{
					// Default UVs
					for (size_t i = 0; i < vertices.size(); i++)
					{
						vertices[i].uv = glm::vec2(0.0f, 0.0f);
					}
				}

				// Extract indices
				std::vector<uint32_t> indices;
				if (primitive.indices >= 0)
				{
					const auto &index_accessor = model.accessors[primitive.indices];
					const auto &index_view     = model.bufferViews[index_accessor.bufferView];
					const auto &index_buffer   = model.buffers[index_view.buffer];

					// Extract indices - convert from any type to uint32_t
					indices.resize(index_accessor.count);

					const unsigned char *data = &index_buffer.data[index_view.byteOffset + index_accessor.byteOffset];
					switch (index_accessor.componentType)
					{
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
						{
							for (size_t i = 0; i < index_accessor.count; i++)
							{
								indices[i] = static_cast<uint32_t>(data[i]) + vertex_offset;
							}
							break;
						}
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
						{
							const uint16_t *short_data = reinterpret_cast<const uint16_t *>(data);
							for (size_t i = 0; i < index_accessor.count; i++)
							{
								indices[i] = static_cast<uint32_t>(short_data[i]) + vertex_offset;
							}
							break;
						}
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
						{
							const uint32_t *int_data = reinterpret_cast<const uint32_t *>(data);
							for (size_t i = 0; i < index_accessor.count; i++)
							{
								indices[i] = int_data[i] + vertex_offset;
							}
							break;
						}
						default:
							throw std::runtime_error("Unsupported index component type");
					}
				}
				else
				{
					// If no indices provided, create sequential ones
					indices.resize(vertices.size());
					for (size_t i = 0; i < vertices.size(); i++)
					{
						indices[i] = static_cast<uint32_t>(i) + vertex_offset;
					}
				}

				// Add to our collected data
				all_vertices.insert(all_vertices.end(), vertices.begin(), vertices.end());
				all_indices.insert(all_indices.end(), indices.begin(), indices.end());

				// Update vertex offset
				vertex_offset += static_cast<uint32_t>(vertices.size());
			}
		}

		// Recursively process child nodes
		for (int child : node.children)
		{
			process_node(child, node_transform);
		}
	};

	// Process all root nodes in the scene
	const auto &scene = model.scenes[scene_index];
	for (int node_index : scene.nodes)
	{
		process_node(node_index, glm::mat4(1.0f));
	}

	// Throw error if no data was loaded
	if (all_vertices.empty())
	{
		throw std::runtime_error("No valid mesh data found in GLTF file");
	}

	// Optimize the mesh
	size_t                index_count = all_indices.size();
	std::vector<uint32_t> remap(index_count);
	size_t                vertex_count = meshopt_generateVertexRemap(remap.data(), all_indices.data(), index_count, all_vertices.data(), all_vertices.size(), sizeof(lz::Vertex));

	std::vector<lz::Vertex> tmp_vertices(vertex_count);
	std::vector<uint32_t>   tmp_indices(index_count);

	meshopt_remapVertexBuffer(tmp_vertices.data(), all_vertices.data(), all_vertices.size(), sizeof(lz::Vertex), remap.data());
	meshopt_remapIndexBuffer(tmp_indices.data(), all_indices.data(), index_count, remap.data());

	meshopt_optimizeVertexCache(tmp_indices.data(), tmp_indices.data(), index_count, vertex_count);
	meshopt_optimizeVertexFetch(tmp_vertices.data(), tmp_indices.data(), index_count, tmp_vertices.data(), vertex_count, sizeof(lz::Vertex));

	mesh_data.vertices = std::move(tmp_vertices);
	mesh_data.indices  = std::move(tmp_indices);

	mesh_data.sphere_bound = glm::vec4(0.5f * (min_bound + max_bound), glm::length(max_bound - min_bound) * 0.5f);

	return mesh_data;
}

void MeshData::append_meshlets(std::vector<Meshlet> &meshlets_datum, std::vector<uint32_t> &meshlet_data_datum, const uint32_t vertex_offset)
{
	assert(primitive_topology == vk::PrimitiveTopology::eTriangleList);

	std::vector<meshopt_Meshlet> tmp_meshlets(meshopt_buildMeshletsBound(indices.size(), k_max_vertices, k_max_triangles));
	std::vector<unsigned int>    meshlet_vertices(tmp_meshlets.size() * k_max_vertices);
	std::vector<unsigned char>   meshlet_triangles(tmp_meshlets.size() * k_max_triangles * 3);

	tmp_meshlets.resize(meshopt_buildMeshlets(tmp_meshlets.data(),
	                                          meshlet_vertices.data(), meshlet_triangles.data(),
	                                          indices.data(), indices.size(),
	                                          &vertices[0].pos.x, vertices.size(), sizeof(Vertex),
	                                          k_max_vertices, k_max_triangles, k_cone_weight));

	for (auto &meshlet : tmp_meshlets)
	{
		uint32_t data_offset = uint32_t(meshlet_data_datum.size());

		for (size_t i = 0; i < meshlet.vertex_count; ++i)
		{
			meshlet_data_datum.push_back(meshlet_vertices[meshlet.vertex_offset + i]);
		}

		const unsigned int *index_groups = reinterpret_cast<const unsigned int *>(&meshlet_triangles[0] + meshlet.triangle_offset);
		// round up to multiple of 4
		unsigned int index_group_count = (meshlet.triangle_count * 3 + 3) / 4;

		for (size_t i = 0; i < index_group_count; ++i)
		{
			meshlet_data_datum.push_back(index_groups[i]);
		}

		// TODO: add meshlet bounding data

		Meshlet m        = {};
		m.data_offset    = data_offset;
		m.vertex_offset  = vertex_offset;
		m.triangle_count = meshlet.triangle_count;
		m.vertex_count   = meshlet.vertex_count;

		meshlets_datum.push_back(m);
	}
}

lz::Vertex MeshData::triangle_vertex_sample(Vertex triangle_vertices[3], glm::vec2 rand_val)
{
	float sqx = sqrt(rand_val.x);
	float y   = rand_val.y;

	const float weights[] = {1.0f - sqx, sqx * (1.0f - y), y * sqx};

	Vertex res = {glm::vec3(0.0), glm::vec3(0.0f), glm::vec2(0.0f)};

	for (size_t vertex_number = 0; vertex_number < 3; ++vertex_number)
	{
		res.pos += triangle_vertices[vertex_number].pos * weights[vertex_number];
		res.normal += triangle_vertices[vertex_number].normal * weights[vertex_number];
		res.uv += triangle_vertices[vertex_number].uv * weights[vertex_number];
	}
	return res;
}

Mesh::Mesh(const MeshData &mesh_data) :
    mesh_data(mesh_data), indices_count(mesh_data.indices.size()), vertices_count(mesh_data.vertices.size()), primitive_topology(mesh_data.primitive_topology)
{
}

// Add second constructor for Mesh to load directly from file
Mesh::Mesh(const std::string &file_name, glm::vec3 scale) :
    mesh_data(file_name, scale), indices_count(mesh_data.indices.size()), vertices_count(mesh_data.vertices.size()), primitive_topology(mesh_data.primitive_topology)
{
}

lz::VertexDeclaration Mesh::get_vertex_declaration()
{
	lz::VertexDeclaration vertex_decl;
	// interleaved variant
	vertex_decl.add_vertex_input_binding(0, sizeof(lz::Vertex));
	vertex_decl.add_vertex_attribute(0, offsetof(lz::Vertex, pos), lz::VertexDeclaration::AttribTypes::eVec3, 0);
	vertex_decl.add_vertex_attribute(0, offsetof(lz::Vertex, normal), lz::VertexDeclaration::AttribTypes::eVec3, 1);
	vertex_decl.add_vertex_attribute(0, offsetof(lz::Vertex, uv), lz::VertexDeclaration::AttribTypes::eVec2, 2);
	// separate buffers variant
	/*vertexDecl.add_vertex_input_binding(0, sizeof(glm::vec3));
	vertexDecl.add_vertex_attribute(0, 0, lz::VertexDeclaration::AttribTypes::eVec3, 0);
	vertexDecl.add_vertex_input_binding(1, sizeof(glm::vec3));
	vertexDecl.add_vertex_attribute(1, 0, lz::VertexDeclaration::AttribTypes::eVec3, 1);
	vertexDecl.add_vertex_input_binding(2, sizeof(glm::vec2));
	vertexDecl.add_vertex_attribute(2, 0, lz::VertexDeclaration::AttribTypes::eVec2, 2);*/

	return vertex_decl;
}
}        // namespace lz
