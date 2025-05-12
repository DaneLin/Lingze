#include "MeshLoader.h"
#include <filesystem>

#include "backend/Logging.h"

#include "tiny_gltf.h"
#include "tiny_obj_loader.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace lz
{

//----------------------------------------
// Obj_loader implementation
//----------------------------------------

bool ObjMeshLoader::can_load(const std::string &file_name)
{
	std::filesystem::path path(file_name);
	std::string           ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	return ext == ".obj";
}

Mesh ObjMeshLoader::load()
{
	Mesh mesh;

	tinyobj::attrib_t                attrib;
	std::vector<tinyobj::shape_t>    shapes;
	std::vector<tinyobj::material_t> materials;
	std::string                      warn;
	std::string                      err;

	// Get base directory for material files
	std::string base_dir = std::filesystem::path(file_path).parent_path().string();
	if (!base_dir.empty() && base_dir.back() != '/' && base_dir.back() != '\\')
		base_dir += '/';

	// Load the OBJ file
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file_path.c_str(), base_dir.c_str(), true);

	if (!warn.empty())
	{
		LOGW("obj warning: {}", warn);
	}

	if (!err.empty())
	{
		LOGE("obj error: {}", err);
	}

	if (!ret)
	{
		throw std::runtime_error("load obj file error: " + file_path);
	}
	LOGI("load obj file success: {}", file_path);

	// Process each shape in the OBJ file
	for (size_t s = 0; s < shapes.size(); s++)
	{
		SubMesh sub_mesh;

		// Set material name if available
		if (!shapes[s].mesh.material_ids.empty())
		{
			int material_id = shapes[s].mesh.material_ids[0];        // Use first material
			if (material_id >= 0 && material_id < static_cast<int>(materials.size()))
			{
				sub_mesh.material_name = materials[material_id].name;
			}
		}

		// Count unique vertices using face indices
		size_t                index_offset = 0;
		std::vector<Vertex>   temp_vertices;
		std::vector<uint32_t> temp_indices;

		// For each face in the shape
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
		{
			// Triangulate polygon if needed - OBJ can have faces with more than 3 vertices
			int fv = shapes[s].mesh.num_face_vertices[f];

			// For each vertex in the face
			for (int v = 0; v < fv; v++)
			{
				// Get index data
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

				// Create a new vertex
				Vertex vertex;

				// Position
				if (idx.vertex_index >= 0)
				{
					vertex.pos.x = attrib.vertices[3 * idx.vertex_index + 0];
					vertex.pos.y = attrib.vertices[3 * idx.vertex_index + 1];
					vertex.pos.z = attrib.vertices[3 * idx.vertex_index + 2];
				}

				// Normal
				if (idx.normal_index >= 0)
				{
					vertex.normal.x = attrib.normals[3 * idx.normal_index + 0];
					vertex.normal.y = attrib.normals[3 * idx.normal_index + 1];
					vertex.normal.z = attrib.normals[3 * idx.normal_index + 2];
				}
				else
				{
					// Default normal
					vertex.normal = glm::vec3(0.0f, 0.0f, 1.0f);
				}

				// Texture coordinates
				if (idx.texcoord_index >= 0)
				{
					vertex.uv.x = attrib.texcoords[2 * idx.texcoord_index + 0];
					vertex.uv.y = attrib.texcoords[2 * idx.texcoord_index + 1];
				}
				else
				{
					// Default texture coordinates
					vertex.uv = glm::vec2(0.0f, 0.0f);
				}

				// For triangulation - convert polygons to triangles
				if (v >= 3)
				{
					// Add vertices for triangulation (0, n-1, n)
					temp_indices.push_back(temp_indices[temp_indices.size() - fv]);        // First vertex
					temp_indices.push_back(temp_indices[temp_indices.size() - 1]);         // Previous vertex
					temp_vertices.push_back(vertex);
					temp_indices.push_back(uint32_t(temp_indices.size() - 1));        // Current vertex
				}
				else
				{
					// Add normal vertex
					temp_vertices.push_back(vertex);
					temp_indices.push_back(uint32_t(temp_indices.size()));
				}
			}

			index_offset += fv;
		}

		// Assign vertices and indices to submesh
		sub_mesh.vertices = std::move(temp_vertices);
		sub_mesh.indices  = std::move(temp_indices);

		// Set primitive topology to triangle list
		sub_mesh.primitive_topology = vk::PrimitiveTopology::eTriangleList;

		// Optimize submesh
		sub_mesh.optimize();

		// Add to mesh
		mesh.add_sub_mesh(sub_mesh);
	}
	return mesh;
}

//----------------------------------------
// Gltf_loader implementation
//----------------------------------------

bool GltfMeshLoader::can_load(const std::string &file_name)
{
	std::filesystem::path path(file_name);
	std::string           ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	return ext == ".gltf" || ext == ".glb";
}

Mesh GltfMeshLoader::load()
{
	tinygltf::Model    model;
	tinygltf::TinyGLTF loader;
	std::string        err;
	std::string        warn;

	bool        ret = false;
	std::string ext = std::filesystem::path(file_path).extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	if (ext == ".glb")
	{
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, file_path);
	}
	else
	{
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, file_path);
	}

	if (!warn.empty())
	{
		LOGW("gltf warning: {}", warn);
	}

	if (!err.empty())
	{
		LOGE("gltf error: {}", err);
	}

	if (!ret)
	{
		throw std::runtime_error("load gltf file error: " + file_path);
	}
	LOGI("load gltf file success: {}", file_path);

	Mesh mesh;

	// load materials and textures
	load_materials_and_textures(model, mesh);

	const auto &scene = model.scenes[model.defaultScene];

	// process node recursively
	std::function<void(int, const glm::mat4 &)> process_node =
	    [&](int node_index, const glm::mat4 &parent_transform) {
		    const auto &node = model.nodes[node_index];

		    // calculate current node's transform matrix
		    glm::mat4 local_transform(1.0f);

		    if (node.matrix.size() == 16)
		    {
			    // use matrix directly
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
		    else
		    {
			    // use TRS (translation, rotation, scale)
			    glm::vec3 translation = glm::vec3(0.0f);
			    glm::quat rotation    = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
			    glm::vec3 scale_local = glm::vec3(1.0f);

			    if (node.translation.size() == 3)
			    {
				    translation = glm::vec3(
				        static_cast<float>(node.translation[0]),
				        static_cast<float>(node.translation[1]),
				        static_cast<float>(node.translation[2]));
			    }

			    if (node.rotation.size() == 4)
			    {
				    rotation = glm::quat(
				        static_cast<float>(node.rotation[3]),        // w
				        static_cast<float>(node.rotation[0]),        // x
				        static_cast<float>(node.rotation[1]),        // y
				        static_cast<float>(node.rotation[2])         // z
				    );
			    }

			    if (node.scale.size() == 3)
			    {
				    scale_local = glm::vec3(
				        static_cast<float>(node.scale[0]),
				        static_cast<float>(node.scale[1]),
				        static_cast<float>(node.scale[2]));
			    }

			    // build transform matrix
			    glm::mat4 translate_mat = glm::translate(glm::mat4(1.0f), translation);
			    glm::mat4 rotate_mat    = glm::toMat4(rotation);
			    glm::mat4 scale_mat     = glm::scale(glm::mat4(1.0f), scale_local);

			    local_transform = translate_mat * rotate_mat * scale_mat;
		    }

		    // apply parent transform
		    glm::mat4 node_transform = parent_transform * local_transform;

		    // process the mesh of the node
		    if (node.mesh >= 0)
		    {
			    const auto &mesh_data = model.meshes[node.mesh];

			    // process all primitives of the mesh
			    for (const auto &primitive : mesh_data.primitives)
			    {
				    SubMesh sub_mesh;

				    // set material name
				    if (primitive.material >= 0)
				    {
					    const auto &material = model.materials[primitive.material];
					    // Use the same naming convention as in load_materials_and_textures
					    std::string mat_name = material.name.empty() ? "material_" + std::to_string(primitive.material) : material.name;
					    sub_mesh.material_name = mat_name;
					    sub_mesh.material = mesh.get_material(mat_name);
				    }

				    // get vertex count
				    const auto &pos_accessor_iter = primitive.attributes.find("POSITION");
				    if (pos_accessor_iter == primitive.attributes.end())
				    {
					    continue;        // skip primitives without position attribute
				    }

				    const auto &pos_accessor = model.accessors[pos_accessor_iter->second];
				    const auto &pos_view     = model.bufferViews[pos_accessor.bufferView];
				    const auto &pos_buffer   = model.buffers[pos_view.buffer];

				    size_t              vertices_count = pos_accessor.count;
				    std::vector<Vertex> vertices(vertices_count);

				    // process position data
				    {
					    const float *pos_data   = reinterpret_cast<const float *>(&pos_buffer.data[pos_view.byteOffset + pos_accessor.byteOffset]);
					    const size_t pos_stride = pos_view.byteStride ? pos_view.byteStride / sizeof(float) : 3;

					    for (size_t i = 0; i < vertices_count; i++)
					    {
						    glm::vec3 pos(
						        pos_data[i * pos_stride + 0],
						        pos_data[i * pos_stride + 1],
						        pos_data[i * pos_stride + 2]);

						    // apply node transform
						    glm::vec4 transformed_pos = node_transform * glm::vec4(pos, 1.0f);
						    vertices[i].pos           = glm::vec3(transformed_pos) / transformed_pos.w;
					    }
				    }

				    // process normal data
				    const auto &normal_accessor_iter = primitive.attributes.find("NORMAL");
				    if (normal_accessor_iter != primitive.attributes.end())
				    {
					    const auto &normal_accessor = model.accessors[normal_accessor_iter->second];
					    const auto &normal_view     = model.bufferViews[normal_accessor.bufferView];
					    const auto &normal_buffer   = model.buffers[normal_view.buffer];

					    const float *normal_data   = reinterpret_cast<const float *>(&normal_buffer.data[normal_view.byteOffset + normal_accessor.byteOffset]);
					    const size_t normal_stride = normal_view.byteStride ? normal_view.byteStride / sizeof(float) : 3;

					    for (size_t i = 0; i < vertices_count; i++)
					    {
						    glm::vec3 normal(
						        normal_data[i * normal_stride + 0],
						        normal_data[i * normal_stride + 1],
						        normal_data[i * normal_stride + 2]);

						    // apply node rotation (not include scale and translation)
						    glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(node_transform)));
						    vertices[i].normal      = glm::normalize(normal_matrix * normal);
					    }
				    }
				    else
				    {
					    // default normal
					    for (size_t i = 0; i < vertices_count; i++)
					    {
						    vertices[i].normal = glm::vec3(0.0f, 0.0f, 1.0f);
					    }
				    }

				    // process texture coordinates
				    const auto &texcoord_accessor_iter = primitive.attributes.find("TEXCOORD_0");
				    if (texcoord_accessor_iter != primitive.attributes.end())
				    {
					    const auto &texcoord_accessor = model.accessors[texcoord_accessor_iter->second];
					    const auto &texcoord_view     = model.bufferViews[texcoord_accessor.bufferView];
					    const auto &texcoord_buffer   = model.buffers[texcoord_view.buffer];

					    const float *texcoord_data   = reinterpret_cast<const float *>(&texcoord_buffer.data[texcoord_view.byteOffset + texcoord_accessor.byteOffset]);
					    const size_t texcoord_stride = texcoord_view.byteStride ? texcoord_view.byteStride / sizeof(float) : 2;

					    for (size_t i = 0; i < vertices_count; i++)
					    {
						    vertices[i].uv.x = texcoord_data[i * texcoord_stride + 0];
						    vertices[i].uv.y = texcoord_data[i * texcoord_stride + 1];
					    }
				    }
				    else
				    {
					    // default texture coordinates
					    for (size_t i = 0; i < vertices_count; i++)
					    {
						    vertices[i].uv = glm::vec2(0.0f, 0.0f);
					    }
				    }

				    // process indices
				    if (primitive.indices >= 0)
				    {
					    const auto &index_accessor = model.accessors[primitive.indices];
					    const auto &index_view     = model.bufferViews[index_accessor.bufferView];
					    const auto &index_buffer   = model.buffers[index_view.buffer];

					    size_t index_count = index_accessor.count;
					    sub_mesh.indices.resize(index_count);

					    const unsigned char *data = &index_buffer.data[index_view.byteOffset + index_accessor.byteOffset];

					    // process different types of indices
					    switch (index_accessor.componentType)
					    {
						    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
						    {
							    for (size_t i = 0; i < index_count; i++)
							    {
								    sub_mesh.indices[i] = static_cast<uint32_t>(data[i]);
							    }
							    break;
						    }
						    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
						    {
							    const uint16_t *short_data = reinterpret_cast<const uint16_t *>(data);
							    for (size_t i = 0; i < index_count; i++)
							    {
								    sub_mesh.indices[i] = static_cast<uint32_t>(short_data[i]);
							    }
							    break;
						    }
						    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
						    {
							    const uint32_t *int_data = reinterpret_cast<const uint32_t *>(data);
							    for (size_t i = 0; i < index_count; i++)
							    {
								    sub_mesh.indices[i] = int_data[i];
							    }
							    break;
						    }
						    default:
							    throw std::runtime_error("unsupported index component type");
					    }
				    }
				    else
				    {
					    // no indices, create continuous indices
					    sub_mesh.indices.resize(vertices_count);
					    for (size_t i = 0; i < vertices_count; i++)
					    {
						    sub_mesh.indices[i] = static_cast<uint32_t>(i);
					    }
				    }

				    // set primitive topology
				    switch (primitive.mode)
				    {
					    case TINYGLTF_MODE_TRIANGLES:
						    sub_mesh.primitive_topology = vk::PrimitiveTopology::eTriangleList;
						    break;
					    case TINYGLTF_MODE_TRIANGLE_STRIP:
						    sub_mesh.primitive_topology = vk::PrimitiveTopology::eTriangleStrip;
						    break;
					    case TINYGLTF_MODE_TRIANGLE_FAN:
						    sub_mesh.primitive_topology = vk::PrimitiveTopology::eTriangleFan;
						    break;
					    case TINYGLTF_MODE_LINE:
						    sub_mesh.primitive_topology = vk::PrimitiveTopology::eLineList;
						    break;
					    case TINYGLTF_MODE_LINE_STRIP:
						    sub_mesh.primitive_topology = vk::PrimitiveTopology::eLineStrip;
						    break;
					    case TINYGLTF_MODE_POINTS:
						    sub_mesh.primitive_topology = vk::PrimitiveTopology::ePointList;
						    break;
					    default:
						    sub_mesh.primitive_topology = vk::PrimitiveTopology::eTriangleList;
						    break;
				    }

				    // set vertices data
				    sub_mesh.vertices = std::move(vertices);

				    // optimize sub mesh
				    sub_mesh.optimize();

				    // add to mesh
				    mesh.add_sub_mesh(sub_mesh);
			    }
		    }

		    // process child nodes recursively
		    for (int child : node.children)
		    {
			    process_node(child, node_transform);
		    }
	    };

	// process from root node
	for (int node_index : scene.nodes)
	{
		process_node(node_index, glm::mat4(1.0f));
	}
	return mesh;
}

void GltfMeshLoader::load_materials_and_textures(const tinygltf::Model &model, Mesh &mesh)
{
	// load all textures
	std::vector<std::shared_ptr<Texture>> textures;
	textures.resize(model.textures.size());

	for (size_t i = 0; i < model.textures.size(); i++)
	{
		const auto &gltf_texture = model.textures[i];
		if (gltf_texture.source >= 0 && gltf_texture.source < model.images.size())
		{
			const auto &image   = model.images[gltf_texture.source];
			auto        texture = std::make_shared<Texture>();

			// set texture basic info
			texture->name     = image.name.empty() ? "texture_" + std::to_string(i) : image.name;
			texture->width    = image.width;
			texture->height   = image.height;
			texture->channels = image.component;
			texture->uri      = image.uri;

			// copy image data
			texture->data = image.image;

			// store texture
			textures[i] = texture;

			DLOGI("Loaded texture: {} ({}x{}, {} channels)", texture->name, texture->width, texture->height, texture->channels);

		}
	}

	// load materials
	for (size_t i = 0; i < model.materials.size(); i++)
	{
		const auto &gltf_material = model.materials[i];
		auto        material      = std::make_shared<Material>();

		// set material name
		material->name = gltf_material.name.empty() ? "material_" + std::to_string(material_count_++) : gltf_material.name;

		// process PBR material parameters
		if (gltf_material.pbrMetallicRoughness.baseColorFactor.size() == 4)
		{
			material->base_color_factor = glm::vec4(
			    static_cast<float>(gltf_material.pbrMetallicRoughness.baseColorFactor[0]),
			    static_cast<float>(gltf_material.pbrMetallicRoughness.baseColorFactor[1]),
			    static_cast<float>(gltf_material.pbrMetallicRoughness.baseColorFactor[2]),
			    static_cast<float>(gltf_material.pbrMetallicRoughness.baseColorFactor[3]));
		}

		material->metallic_factor  = static_cast<float>(gltf_material.pbrMetallicRoughness.metallicFactor);
		material->roughness_factor = static_cast<float>(gltf_material.pbrMetallicRoughness.roughnessFactor);

		if (gltf_material.emissiveFactor.size() == 3)
		{
			material->emissive_factor = glm::vec3(
			    static_cast<float>(gltf_material.emissiveFactor[0]),
			    static_cast<float>(gltf_material.emissiveFactor[1]),
			    static_cast<float>(gltf_material.emissiveFactor[2]));
		}

		// associate textures
		// diffuse/base color texture
		if (gltf_material.pbrMetallicRoughness.baseColorTexture.index >= 0 &&
		    gltf_material.pbrMetallicRoughness.baseColorTexture.index < textures.size())
		{
			material->diffuse_texture = textures[gltf_material.pbrMetallicRoughness.baseColorTexture.index];
			DLOGI("Material {} uses diffuse texture {}", material->name, material->diffuse_texture->name);
		}

		// metallic/roughness texture
		if (gltf_material.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0 &&
		    gltf_material.pbrMetallicRoughness.metallicRoughnessTexture.index < textures.size())
		{
			material->metallic_roughness_texture = textures[gltf_material.pbrMetallicRoughness.metallicRoughnessTexture.index];
			DLOGI("Material {} uses metallic-roughness texture {}", material->name, material->metallic_roughness_texture->name);
		}

		// normal texture
		if (gltf_material.normalTexture.index >= 0 &&
		    gltf_material.normalTexture.index < textures.size())
		{
			material->normal_texture = textures[gltf_material.normalTexture.index];
			DLOGI("Material {} uses normal texture {}", material->name, material->normal_texture->name);
		}

		// emissive texture
		if (gltf_material.emissiveTexture.index >= 0 &&
		    gltf_material.emissiveTexture.index < textures.size())
		{
			material->emissive_texture = textures[gltf_material.emissiveTexture.index];
			DLOGI("Material {} uses emissive texture {}", material->name, material->emissive_texture->name);
		}

		// occlusion texture
		if (gltf_material.occlusionTexture.index >= 0 &&
		    gltf_material.occlusionTexture.index < textures.size())
		{
			material->occlusion_texture = textures[gltf_material.occlusionTexture.index];
			DLOGI("Material {} uses occlusion texture {}", material->name, material->occlusion_texture->name);
		}

		// add to model's material list
		mesh.add_material(material);
		DLOGI("Added material: {}", material->name);
	}
}

//----------------------------------------
// MeshLoaderManager implementation
//----------------------------------------

MeshLoaderManager::MeshLoaderManager()
{
	loaders_.push_back(std::make_shared<ObjMeshLoader>());
	loaders_.push_back(std::make_shared<GltfMeshLoader>());
}

MeshLoaderManager::~MeshLoaderManager()
{
	
}

MeshLoaderManager& MeshLoaderManager::get_instance()
{
	static MeshLoaderManager instance_;
	return instance_;
}

Mesh MeshLoaderManager::load(const std::string &file_name)
{
	auto loader = get_loader(file_name);
	return loader->load();
}

std::shared_ptr<MeshLoader> MeshLoaderManager::get_loader(const std::string &file_name)
{

	std::filesystem::path path(file_name);
	std::string           ext = path.extension().string();

	for (auto &loader : loaders_)
	{
		if (loader->can_load(file_name))
		{
			loader->set_file_path(file_name);
			return loader;
		}
	}

	throw std::runtime_error("Unsupported file extension: " + ext);
	return nullptr;
}

}        // namespace lz