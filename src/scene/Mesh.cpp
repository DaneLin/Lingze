#include "Mesh.h"
#include "meshoptimizer.h"
#include "tiny_obj_loader.h"
#include "../backend/Logging.h"
#include <iostream>

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
	this->primitive_topology = vk::PrimitiveTopology::eTriangleList;
	tinyobj::attrib_t                attrib;
	std::vector<tinyobj::shape_t>    shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;

	LOGI("Loading mesh: {}", file_name);
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

	std::vector<Vertex> triangle_vertices;
	for (auto &shape : shapes)
	{
		for (auto &index : shape.mesh.indices)
		{
			Vertex vertex;
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
	size_t                vertex_count = meshopt_generateVertexRemap(remap.data(), 0, index_count, triangle_vertices.data(), index_count, sizeof(Vertex));

	std::vector<Vertex>   tmp_vertices(vertex_count);
	std::vector<uint32_t> tmp_indices(index_count);

	meshopt_remapVertexBuffer(tmp_vertices.data(), triangle_vertices.data(), index_count, sizeof(Vertex), remap.data());
	meshopt_remapIndexBuffer(tmp_indices.data(), 0, index_count, remap.data());

	meshopt_optimizeVertexCache(tmp_indices.data(), tmp_indices.data(), index_count, vertex_count);
	meshopt_optimizeVertexFetch(tmp_vertices.data(), tmp_indices.data(), index_count, tmp_vertices.data(), vertex_count, sizeof(Vertex));

	this->vertices = std::move(tmp_vertices);
	this->indices  = std::move(tmp_indices);

	sphere_bound = glm::vec4(0.5f * (min_bound + max_bound), glm::length(max_bound - min_bound) * 0.5f);
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

MeshData::Vertex MeshData::triangle_vertex_sample(Vertex triangle_vertices[3], glm::vec2 rand_val)
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
{
	this->mesh_data          = mesh_data;
	this->primitive_topology = mesh_data.primitive_topology;
	indices_count            = mesh_data.indices.size();
	vertices_count           = mesh_data.vertices.size();

	vertex_buffer = std::make_unique<lz::StagedBuffer>(physical_device, logical_device, mesh_data.vertices.size() * sizeof(MeshData::Vertex), vk::BufferUsageFlagBits::eVertexBuffer);
	memcpy(vertex_buffer->map(), mesh_data.vertices.data(), sizeof(MeshData::Vertex) * mesh_data.vertices.size());
	vertex_buffer->unmap(transfer_command_buffer);

	if (indices_count > 0)
	{
		index_buffer = std::make_unique<lz::StagedBuffer>(physical_device, logical_device, mesh_data.indices.size() * sizeof(MeshData::IndexType), vk::BufferUsageFlagBits::eIndexBuffer);
		memcpy(index_buffer->map(), mesh_data.indices.data(), sizeof(MeshData::IndexType) * mesh_data.indices.size());
		index_buffer->unmap(transfer_command_buffer);
	}
}

lz::VertexDeclaration Mesh::get_vertex_declaration()
{
	lz::VertexDeclaration vertex_decl;
	// interleaved variant
	vertex_decl.add_vertex_input_binding(0, sizeof(MeshData::Vertex));
	vertex_decl.add_vertex_attribute(0, offsetof(MeshData::Vertex, pos), lz::VertexDeclaration::AttribTypes::eVec3, 0);
	vertex_decl.add_vertex_attribute(0, offsetof(MeshData::Vertex, normal), lz::VertexDeclaration::AttribTypes::eVec3, 1);
	vertex_decl.add_vertex_attribute(0, offsetof(MeshData::Vertex, uv), lz::VertexDeclaration::AttribTypes::eVec2, 2);
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
