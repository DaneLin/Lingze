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
// LzMeshLoader implement
//----------------------------------------

std::shared_ptr<LzMeshLoader> LzMeshLoader::get_loader(const std::string &file_name)
{
	static std::vector<std::shared_ptr<LzMeshLoader>> loaders = {
	    std::make_shared<LzObjLoader>(),
	    std::make_shared<LzGltfLoader>()};

	for (auto &loader : loaders)
	{
		if (loader->can_load(file_name))
		{
			return loader;
		}
	}

	return nullptr;
}

//----------------------------------------
// Obj_loader implement
//----------------------------------------

bool LzObjLoader::can_load(const std::string &file_name)
{
	std::filesystem::path path(file_name);
	std::string           ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	return ext == ".obj";
}

LzMesh LzObjLoader::load(const std::string &file_name)
{
	// tinyobj::ObjReader reader;
	// tinyobj::ObjReaderConfig reader_config;
	// reader_config.triangulate = true;

	// if (!reader.ParseFromFile(file_name, reader_config)) {
	//     if (!reader.Error().empty()) {
	//         LOGE("TinyObjReader error: {}", reader.Error());
	//     }
	//     throw std::runtime_error("load obj file error: " + file_name);
	// }

	// if (!reader.Warning().empty()) {
	//     LOGW("TinyObjReader warning: {}", reader.Warning());
	// }

	// auto& attrib = reader.GetAttrib();
	// auto& shapes = reader.GetShapes();
	// auto& materials = reader.GetMaterials();

	LzMesh mesh;

	// // 处理每个形状
	// for (size_t s = 0; s < shapes.size(); s++) {
	//     SubMesh sub_mesh;

	//     // 如果有材质，保存材质名
	//     if (!shapes[s].mesh.material_ids.empty() && shapes[s].mesh.material_ids[0] >= 0) {
	//         size_t material_id = shapes[s].mesh.material_ids[0];
	//         if (material_id < materials.size()) {
	//             sub_mesh.material_name = materials[material_id].name;
	//         }
	//     }

	//     // 遍历所有面
	//     size_t index_offset = 0;
	//     std::unordered_map<tinyobj::index_t, uint32_t, tinyobj::index_t_hash> unique_vertices;

	//     for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
	//         size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

	//         // 处理面的顶点
	//         for (size_t v = 0; v < fv; v++) {
	//             tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

	//             // 检查是否已经存在这个顶点
	//             if (unique_vertices.count(idx) == 0) {
	//                 // 创建新顶点
	//                 LzVertex vertex;

	//                 // 读取位置
	//                 if (idx.vertex_index >= 0) {
	//                     vertex.pos = {
	//                         attrib.vertices[3 * size_t(idx.vertex_index) + 0] * scale.x,
	//                         attrib.vertices[3 * size_t(idx.vertex_index) + 1] * scale.y,
	//                         attrib.vertices[3 * size_t(idx.vertex_index) + 2] * scale.z
	//                     };
	//                 }

	//                 // 读取法线
	//                 if (idx.normal_index >= 0) {
	//                     vertex.normal = {
	//                         attrib.normals[3 * size_t(idx.normal_index) + 0],
	//                         attrib.normals[3 * size_t(idx.normal_index) + 1],
	//                         attrib.normals[3 * size_t(idx.normal_index) + 2]
	//                     };
	//                 } else {
	//                     vertex.normal = {0.0f, 0.0f, 1.0f}; // 默认法线
	//                 }

	//                 // 读取纹理坐标
	//                 if (idx.texcoord_index >= 0) {
	//                     vertex.uv = {
	//                         attrib.texcoords[2 * size_t(idx.texcoord_index) + 0],
	//                         1.0f - attrib.texcoords[2 * size_t(idx.texcoord_index) + 1] // 翻转V坐标
	//                     };
	//                 } else {
	//                     vertex.uv = {0.0f, 0.0f}; // 默认纹理坐标
	//                 }

	//                 // 添加到顶点数组并记录索引
	//                 unique_vertices[idx] = static_cast<uint32_t>(sub_mesh.vertices.size());
	//                 sub_mesh.vertices.push_back(vertex);
	//             }

	//             // 添加索引
	//             sub_mesh.indices.push_back(unique_vertices[idx]);
	//         }

	//         index_offset += fv;
	//     }

	//     // 优化子网格
	//     sub_mesh.optimize();

	//     // 添加到网格
	//     mesh.add_sub_mesh(sub_mesh);
	// }

	return mesh;
}

//----------------------------------------
// Gltf_loader 实现
//----------------------------------------

bool LzGltfLoader::can_load(const std::string &file_name)
{
	std::filesystem::path path(file_name);
	std::string           ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	return ext == ".gltf" || ext == ".glb";
}

LzMesh LzGltfLoader::load(const std::string &file_name)
{
	tinygltf::Model    model;
	tinygltf::TinyGLTF loader;
	std::string        err;
	std::string        warn;

	bool        ret = false;
	std::string ext = std::filesystem::path(file_name).extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	if (ext == ".glb")
	{
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, file_name);
	}
	else
	{
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, file_name);
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
		throw std::runtime_error("load gltf file error: " + file_name);
	}

	LzMesh mesh;

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
					    const auto &material   = model.materials[primitive.material];
					    sub_mesh.material_name = material.name;
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

				    size_t                vertices_count = pos_accessor.count;
				    std::vector<LzVertex> vertices(vertices_count);

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
}        // namespace lz