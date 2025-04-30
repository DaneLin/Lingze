#pragma once

#include <random>

#include "backend/LingzeVK.h"
#include "backend/VertexDeclaration.h"
#include "backend/StagedResources.h"

namespace lz
{
	struct Meshlet
	{
		uint32_t data_offset;
		uint32_t vertex_offset;
		uint8_t triangle_count;
		uint8_t vertex_count;
	};

	struct MeshData
	{
		MeshData();

		MeshData(const std::string file_name, glm::vec3 scale);

		static float get_triangle_area(glm::vec3 points[3]);

		static MeshData generate_point_mesh(MeshData src_mesh, float density);

		static glm::vec2 hammersley_norm(glm::uint i, glm::uint n);

		static MeshData generate_point_mesh_regular(MeshData src_mesh, float density);

		static MeshData generate_point_mesh_sized(MeshData src_mesh, size_t points_per_triangle_count);

		void append_meshlets(std::vector<Meshlet>& meshlets_datum, std::vector<uint32_t>& meshlet_data_datum,const uint32_t vertex_offset);

#pragma pack(push, 1)
		struct Vertex
		{
			glm::vec3 pos;
			glm::vec3 normal;
			glm::vec2 uv;
		};
#pragma pack(pop)

		static Vertex triangle_vertex_sample(Vertex triangle_vertices[3], glm::vec2 rand_val);

		using IndexType = uint32_t;
		std::vector<Vertex> vertices;
		std::vector<IndexType> indices;

		std::vector<Meshlet> meshlets;

		vk::PrimitiveTopology primitive_topology;
	};

	struct Mesh
	{
		Mesh(const MeshData& mesh_data, vk::PhysicalDevice physical_device, vk::Device logical_device,
		     vk::CommandBuffer transfer_command_buffer);
		static lz::VertexDeclaration get_vertex_declaration();

		std::unique_ptr<lz::StagedBuffer> vertex_buffer;
		std::unique_ptr<lz::StagedBuffer> index_buffer;

		MeshData mesh_data;
		size_t indices_count;
		size_t vertices_count;

		vk::PrimitiveTopology primitive_topology;
	};
}
