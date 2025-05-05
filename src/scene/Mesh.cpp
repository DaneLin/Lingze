#include "Mesh.h"
#include "meshoptimizer.h"
#include "tiny_obj_loader.h"
#include "tiny_gltf.h"
#include "../backend/Logging.h"
#include <iostream>
#include <filesystem>

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
	if (!loader) {
		LOGE("Failed to find an appropriate loader for file: {}", file_name);
		throw std::runtime_error("Failed to load mesh: no appropriate loader found");
	}
	
	*this = loader->load(file_name, scale);
}

// Static MeshLoader factory method
std::shared_ptr<MeshLoader> MeshLoader::get_loader(const std::string& file_name)
{
	static std::vector<std::shared_ptr<MeshLoader>> loaders = {
		std::make_shared<ObjLoader>(),
		std::make_shared<GltfLoader>()
	};
	
	for (auto& loader : loaders) {
		if (loader->can_load(file_name)) {
			return loader;
		}
	}
	
	return nullptr;
}

// ObjLoader implementation
bool ObjLoader::can_load(const std::string& file_name)
{
	std::filesystem::path path(file_name);
	std::string ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	return ext == ".obj";
}

MeshData ObjLoader::load(const std::string& file_name, glm::vec3 scale)
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
	size_t vertex_count = meshopt_generateVertexRemap(remap.data(), 0, index_count, triangle_vertices.data(), index_count, sizeof(lz::Vertex));

	std::vector<lz::Vertex>   tmp_vertices(vertex_count);
	std::vector<uint32_t> tmp_indices(index_count);

	meshopt_remapVertexBuffer(tmp_vertices.data(), triangle_vertices.data(), index_count, sizeof(lz::Vertex), remap.data());
	meshopt_remapIndexBuffer(tmp_indices.data(), 0, index_count, remap.data());

	meshopt_optimizeVertexCache(tmp_indices.data(), tmp_indices.data(), index_count, vertex_count);
	meshopt_optimizeVertexFetch(tmp_vertices.data(), tmp_indices.data(), index_count, tmp_vertices.data(), vertex_count, sizeof(lz::Vertex));

	mesh_data.vertices = std::move(tmp_vertices);
	mesh_data.indices = std::move(tmp_indices);

	mesh_data.sphere_bound = glm::vec4(0.5f * (min_bound + max_bound), glm::length(max_bound - min_bound) * 0.5f);
	
	return mesh_data;
}

// GltfLoader implementation
bool GltfLoader::can_load(const std::string& file_name)
{
	std::filesystem::path path(file_name);
	std::string ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	return ext == ".gltf" || ext == ".glb";
}

MeshData GltfLoader::load(const std::string& file_name, glm::vec3 scale)
{
	MeshData mesh_data;
	mesh_data.primitive_topology = vk::PrimitiveTopology::eTriangleList;
	
	LOGI("Loading GLTF mesh: {}", file_name);
	
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;
	
	// Detect if we need to load binary or ASCII glTF
	bool ret = false;
	std::filesystem::path path(file_name);
	if (path.extension() == ".glb") {
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, file_name);
	} else {
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, file_name);
	}
	
	if (!warn.empty()) {
		LOGW("Warning: {}", warn);
	}
	
	if (!err.empty()) {
		LOGE("Error: {}", err);
	}
	
	if (!ret) {
		throw std::runtime_error("Failed to load GLTF mesh");
	}
	
	// For simplicity, we'll load just the first mesh with its first primitive
	if (model.meshes.empty()) {
		throw std::runtime_error("GLTF file contains no meshes");
	}
	
	// Get the first mesh
	const tinygltf::Mesh& gltf_mesh = model.meshes[0];
	
	if (gltf_mesh.primitives.empty()) {
		throw std::runtime_error("GLTF mesh contains no primitives");
	}
	
	// Get the first primitive
	const tinygltf::Primitive& primitive = gltf_mesh.primitives[0];
	
	// Check if we have position data
	if (primitive.attributes.find("POSITION") == primitive.attributes.end()) {
		throw std::runtime_error("GLTF primitive has no POSITION attribute");
	}
	
	// bounding sphere
	glm::vec3 min_bound = glm::vec3(std::numeric_limits<float>::infinity());
	glm::vec3 max_bound = glm::vec3(-std::numeric_limits<float>::infinity());
	
	// Get accessors for vertex data
	const tinygltf::Accessor& pos_accessor = model.accessors[primitive.attributes.at("POSITION")];
	const tinygltf::BufferView& pos_view = model.bufferViews[pos_accessor.bufferView];
	const tinygltf::Buffer& pos_buffer = model.buffers[pos_view.buffer];
	
	// Optional normal and texcoord accessors
	const tinygltf::Accessor* normal_accessor = nullptr;
	const tinygltf::BufferView* normal_view = nullptr;
	const tinygltf::Buffer* normal_buffer = nullptr;
	
	if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
		normal_accessor = &model.accessors[primitive.attributes.at("NORMAL")];
		normal_view = &model.bufferViews[normal_accessor->bufferView];
		normal_buffer = &model.buffers[normal_view->buffer];
	}
	
	const tinygltf::Accessor* texcoord_accessor = nullptr;
	const tinygltf::BufferView* texcoord_view = nullptr;
	const tinygltf::Buffer* texcoord_buffer = nullptr;
	
	if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
		texcoord_accessor = &model.accessors[primitive.attributes.at("TEXCOORD_0")];
		texcoord_view = &model.bufferViews[texcoord_accessor->bufferView];
		texcoord_buffer = &model.buffers[texcoord_view->buffer];
	}
	
	// Get indices
	std::vector<uint32_t> indices;
	if (primitive.indices >= 0) {
		const tinygltf::Accessor& index_accessor = model.accessors[primitive.indices];
		const tinygltf::BufferView& index_view = model.bufferViews[index_accessor.bufferView];
		const tinygltf::Buffer& index_buffer = model.buffers[index_view.buffer];
		
		// Extract indices - convert from whatever type to uint32_t
		indices.resize(index_accessor.count);
		
		const unsigned char* data = &index_buffer.data[index_view.byteOffset + index_accessor.byteOffset];
		switch (index_accessor.componentType) {
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
				for (size_t i = 0; i < index_accessor.count; i++) {
					indices[i] = static_cast<uint32_t>(data[i]);
				}
				break;
			}
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
				const uint16_t* short_data = reinterpret_cast<const uint16_t*>(data);
				for (size_t i = 0; i < index_accessor.count; i++) {
					indices[i] = static_cast<uint32_t>(short_data[i]);
				}
				break;
			}
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
				const uint32_t* int_data = reinterpret_cast<const uint32_t*>(data);
				for (size_t i = 0; i < index_accessor.count; i++) {
					indices[i] = int_data[i];
				}
				break;
			}
			default:
				throw std::runtime_error("Unsupported index component type");
		}
	}
	
	// Create vertices
	std::vector<lz::Vertex> vertices;
	vertices.resize(pos_accessor.count);
	
	// Extract positions
	const float* pos_data = reinterpret_cast<const float*>(&pos_buffer.data[pos_view.byteOffset + pos_accessor.byteOffset]);
	const size_t pos_stride = pos_view.byteStride ? pos_view.byteStride / sizeof(float) : 3;
	
	for (size_t i = 0; i < pos_accessor.count; i++) {
		vertices[i].pos.x = pos_data[i * pos_stride + 0] * scale.x;
		vertices[i].pos.y = pos_data[i * pos_stride + 1] * scale.y;
		vertices[i].pos.z = pos_data[i * pos_stride + 2] * scale.z;
		
		min_bound = glm::min(min_bound, vertices[i].pos);
		max_bound = glm::max(max_bound, vertices[i].pos);
	}
	
	// Extract normals if present
	if (normal_accessor) {
		const float* normal_data = reinterpret_cast<const float*>(&normal_buffer->data[normal_view->byteOffset + normal_accessor->byteOffset]);
		const size_t normal_stride = normal_view->byteStride ? normal_view->byteStride / sizeof(float) : 3;
		
		for (size_t i = 0; i < normal_accessor->count; i++) {
			vertices[i].normal.x = normal_data[i * normal_stride + 0];
			vertices[i].normal.y = normal_data[i * normal_stride + 1];
			vertices[i].normal.z = normal_data[i * normal_stride + 2];
		}
	} else {
		// Default normals
		for (size_t i = 0; i < vertices.size(); i++) {
			vertices[i].normal = glm::vec3(1.0f, 0.0f, 0.0f);
		}
	}
	
	// Extract texture coordinates if present
	if (texcoord_accessor) {
		const float* texcoord_data = reinterpret_cast<const float*>(&texcoord_buffer->data[texcoord_view->byteOffset + texcoord_accessor->byteOffset]);
		const size_t texcoord_stride = texcoord_view->byteStride ? texcoord_view->byteStride / sizeof(float) : 2;
		
		for (size_t i = 0; i < texcoord_accessor->count; i++) {
			vertices[i].uv.x = texcoord_data[i * texcoord_stride + 0];
			vertices[i].uv.y = texcoord_data[i * texcoord_stride + 1];
		}
	} else {
		// Default UVs
		for (size_t i = 0; i < vertices.size(); i++) {
			vertices[i].uv = glm::vec2(0.0f, 0.0f);
		}
	}
	
	// If no indices were provided, create them (just sequential)
	if (indices.empty()) {
		indices.resize(vertices.size());
		for (size_t i = 0; i < vertices.size(); i++) {
			indices[i] = static_cast<uint32_t>(i);
		}
	}
	
	// Optimize the mesh
	size_t index_count = indices.size();
	std::vector<uint32_t> remap(index_count);
	size_t vertex_count = meshopt_generateVertexRemap(remap.data(), indices.data(), index_count, vertices.data(), vertices.size(), sizeof(lz::Vertex));
	
	std::vector<lz::Vertex> tmp_vertices(vertex_count);
	std::vector<uint32_t> tmp_indices(index_count);
	
	meshopt_remapVertexBuffer(tmp_vertices.data(), vertices.data(), vertices.size(), sizeof(lz::Vertex), remap.data());
	meshopt_remapIndexBuffer(tmp_indices.data(), indices.data(), index_count, remap.data());
	
	meshopt_optimizeVertexCache(tmp_indices.data(), tmp_indices.data(), index_count, vertex_count);
	meshopt_optimizeVertexFetch(tmp_vertices.data(), tmp_indices.data(), index_count, tmp_vertices.data(), vertex_count, sizeof(lz::Vertex));
	
	mesh_data.vertices = std::move(tmp_vertices);
	mesh_data.indices = std::move(tmp_indices);
	
	mesh_data.sphere_bound = glm::vec4(0.5f * (min_bound + max_bound), glm::length(max_bound - min_bound) * 0.5f);
	
	return mesh_data;
}

// Add second constructor for Mesh to load directly from file
Mesh::Mesh(const std::string &file_name, glm::vec3 scale, vk::PhysicalDevice physical_device, vk::Device logical_device,
         vk::CommandBuffer transfer_command_buffer)
    : mesh_data(file_name, scale), indices_count(mesh_data.indices.size()), vertices_count(mesh_data.vertices.size()),
      primitive_topology(mesh_data.primitive_topology)
{
}

float MeshData::get_triangle_area(glm::vec3 points[3])
{
	glm::vec3 a = points[0];
	glm::vec3 b = points[1];
	glm::vec3 c = points[2];

	glm::vec3 ab = b - a;
	glm::vec3 ac = c - a;

	return glm::length(glm::cross(ab, ac)) / 2.0f;
}

MeshData MeshData::generate_point_mesh(MeshData src_mesh, float density)
{
	assert(src_mesh.primitive_topology == vk::PrimitiveTopology::eTriangleList);
	const IndexType triangles_count = IndexType(src_mesh.indices.size() / 3);

	std::vector<float> triangle_areas;
	triangle_areas.resize(triangles_count);

	float total_area = 0.0f;
	for (size_t triangle_index = 0; triangle_index < triangles_count; ++triangle_index)
	{
		glm::vec3 points[3];
		for (size_t vertex_number = 0; vertex_number < 3; ++vertex_number)
		{
			points[vertex_number] = src_mesh.vertices[src_mesh.indices[triangle_index * 3 + vertex_number]].pos;
		}
		float area = get_triangle_area(points);
		total_area += area;
		triangle_areas[triangle_index] = total_area;
	}

	size_t points_count = size_t(total_area * density);

	static std::default_random_engine            eng;
	static std::uniform_real_distribution<float> dis(0.0f, 1.f);

	MeshData res;
	res.primitive_topology = vk::PrimitiveTopology::ePointList;
	for (size_t point_index = 0; point_index < points_count; ++point_index)
	{
		float area_val = dis(eng);

		auto it = std::lower_bound(triangle_areas.begin(), triangle_areas.end(), total_area);
		if (it == triangle_areas.end())
		{
			continue;
		}

		const size_t triangle_index = it - triangle_areas.begin();

		Vertex triangle_vertices[3];
		for (size_t vertex_number = 0; vertex_number < 3; ++vertex_number)
		{
			triangle_vertices[vertex_number] = src_mesh.vertices[src_mesh.indices[triangle_index * 3 + vertex_number]];
		}

		Vertex vertex = triangle_vertex_sample(triangle_vertices, glm::vec2(dis(eng), dis(eng)));
		res.vertices.push_back(vertex);
	}
	return res;
}

glm::vec2 MeshData::hammersley_norm(glm::uint i, glm::uint n)
{
	// principle: reverse bit sequence of i
	glm::uint b = (glm::uint(i) << 16u) | (glm::uint(i) >> 16u);
	b           = (b & 0x55555555u) << 1u | (b & 0xAAAAAAAAu) >> 1u;
	b           = (b & 0x33333333u) << 2u | (b & 0xCCCCCCCCu) >> 2u;
	b           = (b & 0x0F0F0F0Fu) << 4u | (b & 0xF0F0F0F0u) >> 4u;
	b           = (b & 0x00FF00FFu) << 8u | (b & 0xFF00FF00u) >> 8u;

	return glm::vec2(i, b) / glm::vec2(n, 0xffffffffU);
}

MeshData MeshData::generate_point_mesh_regular(MeshData src_mesh, float density)
{
	assert(src_mesh.primitive_topology == vk::PrimitiveTopology::eTriangleList);
	IndexType triangles_count = IndexType(src_mesh.indices.size() / 3);

	std::vector<float> triangle_areas;
	triangle_areas.resize(triangles_count);

	static std::default_random_engine            eng;
	static std::uniform_real_distribution<float> dis(0.0f, 1.f);

	MeshData res;
	res.primitive_topology = vk::PrimitiveTopology::ePointList;

	size_t points_count = 0;
	float  total_area   = 0.0f;

	for (size_t triangle_index = 0; triangle_index < triangles_count; ++triangle_index)
	{
		glm::vec3 points[3];
		for (size_t vertex_number = 0; vertex_number < 3; ++vertex_number)
		{
			points[vertex_number] = src_mesh.vertices[src_mesh.indices[triangle_index * 3 + vertex_number]].pos;
		}

		float area               = get_triangle_area(points);
		float points_count_float = area * density;

		glm::uint points_count = glm::uint(points_count_float);
		float     ratio        = points_count_float - float(points_count);
		points_count += (dis(eng) < ratio) ? 1 : 0;

		Vertex triangle_vertices[3];
		for (size_t vertex_number = 0; vertex_number < 3; ++vertex_number)
		{
			triangle_vertices[vertex_number] = src_mesh.vertices[src_mesh.indices[triangle_index * 3 + vertex_number]];
		}

		for (glm::uint point_number = 0; point_number < points_count; ++point_number)
		{
			Vertex vertex = triangle_vertex_sample(triangle_vertices, hammersley_norm(point_number, points_count));
			vertex.uv.x   = 2.0f / sqrt(density);
			res.vertices.push_back(vertex);
		}
	}
	return res;
}

MeshData MeshData::generate_point_mesh_sized(MeshData src_mesh, size_t points_per_triangle_count)
{
	static std::default_random_engine            eng;
	static std::uniform_real_distribution<float> dis(0.0f, 1.0f);

	assert(src_mesh.primitive_topology == vk::PrimitiveTopology::eTriangleList);
	const IndexType triangles_count = IndexType(src_mesh.indices.size() / 3);

	MeshData res;
	res.primitive_topology = vk::PrimitiveTopology::ePointList;

	for (size_t triangle_index = 0; triangle_index < triangles_count; triangle_index++)
	{
		glm::vec3 points[3];
		for (size_t vertexNumber = 0; vertexNumber < 3; vertexNumber++)
			points[vertexNumber] = src_mesh.vertices[src_mesh.indices[triangle_index * 3 + vertexNumber]].pos;
		float area = get_triangle_area(points);

		Vertex triangle_vertices[3];
		for (size_t vertex_number = 0; vertex_number < 3; vertex_number++)
			triangle_vertices[vertex_number] = src_mesh.vertices[src_mesh.indices[triangle_index * 3 + vertex_number]];

		glm::uint       res_points_count = glm::uint(points_per_triangle_count);
		float           res_point_radius = 2.0f * sqrt(area / points_per_triangle_count);
		constexpr float max_point_radius = 0.6f;        // 0.6f

		if (res_point_radius > max_point_radius)
		{
			res_points_count = glm::uint(res_points_count * std::pow(res_point_radius / max_point_radius, 2.0) + 0.5f);
			res_point_radius = max_point_radius;
		}
		for (glm::uint point_number = 0; point_number < res_points_count; point_number++)
		{
			Vertex vertex = triangle_vertex_sample(triangle_vertices, /*HammersleyNorm(pointNumber, resPointsCount)*/ glm::vec2(dis(eng), dis(eng)));
			vertex.uv.x   = res_point_radius;
			res.vertices.push_back(vertex);
		}
	}
	return res;
}

void MeshData::append_meshlets(std::vector<Meshlet> &meshlets_datum, std::vector<uint32_t> &meshlet_data_datum, const uint32_t vertex_offset)
{
	assert(primitive_topology == vk::PrimitiveTopology::eTriangleList);

	constexpr size_t k_max_vertices  = 64;
	constexpr size_t k_max_triangles = 124;        // we use 4 bytes to store indices, so the max triangle count is 124 that is divisible by 4
	constexpr float  k_cone_weight   = 0.5f;

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

Mesh::Mesh(const MeshData &mesh_data, vk::PhysicalDevice physical_device, vk::Device logical_device,
           vk::CommandBuffer transfer_command_buffer)
    : mesh_data(mesh_data), indices_count(mesh_data.indices.size()), vertices_count(mesh_data.vertices.size()),
      primitive_topology(mesh_data.primitive_topology)
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
