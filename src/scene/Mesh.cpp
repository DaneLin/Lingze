#include "Mesh.h"
#include "backend/Logging.h"
#include "meshoptimizer.h"
#include <algorithm>
#include <limits>

namespace lz
{

//----------------------------------------
// SubMesh implementation
//----------------------------------------

glm::vec4 SubMesh::calculate_bounding_sphere()
{
	if (vertices.empty())
	{
		sphere_bound = glm::vec4(0.0f);
		return sphere_bound;
	}

	glm::vec3 center(0.0f);
	for (const auto &v : vertices)
	{
		center += v.pos;
	}
	center /= static_cast<float>(vertices.size());

	float radius = 0.0f;
	for (const auto &v : vertices)
	{
		float distance = glm::distance(center, v.pos);
		radius         = std::max(radius, distance);
	}

	sphere_bound = glm::vec4(center, radius);
	return sphere_bound;
}

void SubMesh::optimize()
{
	if (vertices.empty() || indices.empty())
	{
		return;
	}

	size_t                index_count = indices.size();
	std::vector<uint32_t> remap(index_count);
	size_t                vertex_count = meshopt_generateVertexRemap(remap.data(), indices.data(), index_count,
	                                                                 vertices.data(), vertices.size(), sizeof(Vertex));

	std::vector<Vertex>   tmp_vertices(vertex_count);
	std::vector<uint32_t> tmp_indices(index_count);

	meshopt_remapVertexBuffer(tmp_vertices.data(), vertices.data(), vertices.size(), sizeof(Vertex), remap.data());
	meshopt_remapIndexBuffer(tmp_indices.data(), indices.data(), index_count, remap.data());

	meshopt_optimizeVertexCache(tmp_indices.data(), tmp_indices.data(), index_count, vertex_count);

	meshopt_optimizeVertexFetch(tmp_vertices.data(), tmp_indices.data(), index_count, tmp_vertices.data(), vertex_count, sizeof(Vertex));

	vertices = std::move(tmp_vertices);
	indices  = std::move(tmp_indices);

	calculate_bounding_sphere();
}

//----------------------------------------
// Mesh implementation
//----------------------------------------

Mesh::Mesh()
{
	mesh_bound = glm::vec4(0.0f);
}

Mesh::Mesh(const std::string &file_name)
{
}

void Mesh::add_sub_mesh(const SubMesh &sub_mesh)
{
	sub_meshes.push_back(sub_mesh);

	calculate_bounding_sphere();
}

SubMesh &Mesh::get_sub_mesh(size_t index)
{
	if (index >= sub_meshes.size())
	{
		throw std::out_of_range("SubMesh index out of range");
	}
	return sub_meshes[index];
}

const SubMesh &Mesh::get_sub_mesh(size_t index) const
{
	if (index >= sub_meshes.size())
	{
		throw std::out_of_range("SubMesh index out of range");
	}
	return sub_meshes[index];
}

glm::vec4 Mesh::calculate_bounding_sphere()
{
	if (sub_meshes.empty())
	{
		mesh_bound = glm::vec4(0.0f);
		return mesh_bound;
	}

	glm::vec3 min_bound(std::numeric_limits<float>::max());
	glm::vec3 max_bound(std::numeric_limits<float>::lowest());

	for (const auto &sub_mesh : sub_meshes)
	{
		for (const auto &vertex : sub_mesh.vertices)
		{
			min_bound = glm::min(min_bound, vertex.pos);
			max_bound = glm::max(max_bound, vertex.pos);
		}
	}

	glm::vec3 center = 0.5f * (min_bound + max_bound);
	float     radius = glm::length(max_bound - min_bound) * 0.5f;

	mesh_bound = glm::vec4(center, radius);
	return mesh_bound;
}

lz::VertexDeclaration Mesh::get_vertex_declaration()
{
	lz::VertexDeclaration vertex_decl;
	vertex_decl.add_vertex_input_binding(0, sizeof(lz::Vertex));
	vertex_decl.add_vertex_attribute(0, offsetof(lz::Vertex, pos), lz::VertexDeclaration::AttribTypes::eVec3, 0);
	vertex_decl.add_vertex_attribute(0, offsetof(lz::Vertex, normal), lz::VertexDeclaration::AttribTypes::eVec3, 1);
	vertex_decl.add_vertex_attribute(0, offsetof(lz::Vertex, uv), lz::VertexDeclaration::AttribTypes::eVec2, 2);
	return vertex_decl;
}

void Mesh::optimize()
{
	for (auto &sub_mesh : sub_meshes)
	{
		sub_mesh.optimize();
	}

	calculate_bounding_sphere();
}

size_t Mesh::get_total_vertex_count() const
{
	size_t count = 0;
	for (const auto &sub_mesh : sub_meshes)
	{
		count += sub_mesh.vertices.size();
	}
	return count;
}

size_t Mesh::get_total_index_count() const
{
	size_t count = 0;
	for (const auto &sub_mesh : sub_meshes)
	{
		count += sub_mesh.indices.size();
	}
	return count;
}

}        // namespace lz
