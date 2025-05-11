#include "RenderContext.h"
#include "backend/Core.h"
#include "backend/Logging.h"
#include "backend/PresentQueue.h"
#include "scene/Mesh.h"

#include "meshoptimizer.h"
#include "scene/Config.h"

namespace lz::render
{

RenderContext::RenderContext(lz::Core *core) :
    core_(core)
{
}

RenderContext::~RenderContext()
{
}

void RenderContext::collect_draw_commands(const lz::Scene *scene)
{
	// Clear previous data
	global_vertices_.clear();
	global_indices_.clear();
	global_vertices_count_ = 0;
	global_indices_count_  = 0;
	mesh_infos_.clear();
	mesh_draws_.clear();

	// Collect renderable entities from the scene
	for (const auto &entity : scene->get_root_entities())
	{
		process_entity(entity);
	}
}

void RenderContext::build_meshlet_data()
{
	// Create a meshlet buffer
	meshlets_.clear();
	meshlet_data_datum_.clear();

	// iterate through all the mesh draws
	for (const auto &mesh_draw : mesh_draws_)
	{
		auto mesh_index = mesh_draw.mesh_index;
		auto mesh_info  = mesh_infos_[mesh_index];

		// build the meshlet data
		std::vector<meshopt_Meshlet> tmp_meshlets(meshopt_buildMeshletsBound(mesh_info.index_count, k_max_vertices, k_max_triangles));
		std::vector<unsigned int>    meshlet_vertices(tmp_meshlets.size() * k_max_vertices);
		std::vector<unsigned char>   meshlet_triangles(tmp_meshlets.size() * k_max_triangles * 3);

		tmp_meshlets.resize(meshopt_buildMeshlets(tmp_meshlets.data(),
		                                          meshlet_vertices.data(), meshlet_triangles.data(),
		                                          global_indices_.data() + mesh_info.index_offset, mesh_info.index_count,
		                                          &(global_vertices_.data() + mesh_info.vertex_offset)->pos.x, mesh_info.vertex_count, sizeof(Vertex),
		                                          k_max_vertices, k_max_triangles, k_cone_weight));

		for (auto &meshlet : tmp_meshlets)
		{
			uint32_t data_offset = uint32_t(meshlet_data_datum_.size());

			for (size_t i = 0; i < meshlet.vertex_count; ++i)
			{
				meshlet_data_datum_.push_back(meshlet_vertices[meshlet.vertex_offset + i]);
			}

			const unsigned int *index_groups = reinterpret_cast<const unsigned int *>(&meshlet_triangles[0] + meshlet.triangle_offset);
			// round up to multiple of 4
			unsigned int index_group_count = (meshlet.triangle_count * 3 + 3) / 4;

			for (size_t i = 0; i < index_group_count; ++i)
			{
				meshlet_data_datum_.push_back(index_groups[i]);
			}

			meshopt_Bounds bounds =
			    meshopt_computeMeshletBounds(&meshlet_vertices[meshlet.vertex_offset],
			                                 &meshlet_triangles[meshlet.triangle_offset],
			                                 meshlet.triangle_count,
			                                 &global_vertices_[mesh_info.vertex_offset].pos.x,
			                                 mesh_info.vertex_count,
			                                 sizeof(Vertex));

			Meshlet m        = {};
			m.sphere_bound   = glm::vec4(bounds.center[0], bounds.center[1], bounds.center[2], bounds.radius);
			m.cone_axis[0]   = bounds.cone_axis_s8[0];
			m.cone_axis[1]   = bounds.cone_axis_s8[1];
			m.cone_axis[2]   = bounds.cone_axis_s8[2];
			m.cone_cutoff    = bounds.cone_cutoff_s8;
			m.data_offset    = data_offset;
			m.vertex_offset  = mesh_info.vertex_offset;
			m.triangle_count = meshlet.triangle_count;
			m.vertex_count   = meshlet.vertex_count;

			meshlets_.push_back(m);
		}
	}
	while (meshlets_.size() % 32 != 0)
	{
		meshlets_.push_back(Meshlet{});
	}
	meshlet_count_ = uint32_t(meshlets_.size());
}

void RenderContext::process_entity(const std::shared_ptr<lz::Entity> &entity)
{
	// Check if the entity has a StaticMeshComponent
	auto *mesh_component = entity->get_component<lz::StaticMeshComponent>();
	if (mesh_component && mesh_component->get_mesh())
	{
		auto *mesh      = mesh_component->get_mesh();
		auto *transform = entity->get_transform();

		// Get the world transformation matrix
		glm::mat4 model_matrix = transform->get_world_matrix();

		// Process each submesh of the mesh
		for (size_t i = 0; i < mesh->get_sub_mesh_count(); ++i)
		{
			const auto &sub_mesh = mesh->get_sub_mesh(i);

			draw_count_++;

			// Collect mesh draw info
			MeshDraw mesh_draw;
			mesh_draw.mesh_index   = uint32_t(mesh_infos_.size());
			mesh_draw.model_matrix = model_matrix;
			mesh_draws_.push_back(mesh_draw);

			// Collect mesh info
			MeshInfo mesh_info;
			mesh_info.sphere_bound  = sub_mesh.sphere_bound;
			mesh_info.vertex_count  = uint32_t(sub_mesh.vertices.size());
			mesh_info.vertex_offset = global_vertices_count_;
			mesh_info.index_offset  = global_indices_count_;
			mesh_info.index_count   = uint32_t(sub_mesh.indices.size());

			mesh_infos_.push_back(mesh_info);

			// Copy vertex and index data to global buffers
			global_vertices_.insert(global_vertices_.end(), sub_mesh.vertices.begin(), sub_mesh.vertices.end());
			global_indices_.insert(global_indices_.end(), sub_mesh.indices.begin(), sub_mesh.indices.end());

			// Update statistics
			global_vertices_count_ += uint32_t(sub_mesh.vertices.size());
			global_indices_count_ += uint32_t(sub_mesh.indices.size());
		}
	}

	// Recursively process child entities
	for (const auto &child : entity->get_children())
	{
		process_entity(child);
	}
}

void RenderContext::create_gpu_resources()
{
	// Create global vertex and index buffers
	auto physical_device = core_->get_physical_device();
	auto logical_device  = core_->get_logical_device();

	// Create a transfer queue for uploading data
	lz::ExecuteOnceQueue transfer_queue(core_);
	auto                 transfer_command_buffer = transfer_queue.begin_command_buffer();

	// Create a mesh vertex buffer
	global_vertex_buffer_ = std::make_unique<lz::StagedBuffer>(
	    physical_device, logical_device,
	    global_vertices_.size() * sizeof(lz::Vertex),
	    vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer);

	if (!global_vertices_.empty())
	{
		memcpy(global_vertex_buffer_->map(), global_vertices_.data(), global_vertices_.size() * sizeof(lz::Vertex));
		global_vertex_buffer_->unmap(transfer_command_buffer);
	}

	// Create a mesh index buffer
	global_index_buffer_ = std::make_unique<lz::StagedBuffer>(
	    physical_device, logical_device,
	    global_indices_.size() * sizeof(uint32_t),
	    vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eStorageBuffer);

	if (!global_indices_.empty())
	{
		memcpy(global_index_buffer_->map(), global_indices_.data(), global_indices_.size() * sizeof(uint32_t));
		global_index_buffer_->unmap(transfer_command_buffer);
	}

	// Create a meshdraw buffer
	mesh_draw_buffer_ = std::make_unique<lz::StagedBuffer>(
	    physical_device, logical_device,
	    mesh_draws_.size() * sizeof(MeshDraw),
	    vk::BufferUsageFlagBits::eStorageBuffer);

	if (!mesh_draws_.empty())
	{
		memcpy(mesh_draw_buffer_->map(), mesh_draws_.data(), mesh_draws_.size() * sizeof(MeshDraw));
		mesh_draw_buffer_->unmap(transfer_command_buffer);
	}

	// Create a meshinfo buffer
	mesh_info_buffer_ = std::make_unique<lz::StagedBuffer>(
	    physical_device, logical_device,
	    mesh_infos_.size() * sizeof(MeshInfo),
	    vk::BufferUsageFlagBits::eStorageBuffer);

	if (!mesh_infos_.empty())
	{
		memcpy(mesh_info_buffer_->map(), mesh_infos_.data(), mesh_infos_.size() * sizeof(MeshInfo));
		mesh_info_buffer_->unmap(transfer_command_buffer);
	}

	// Submit the transfer command
	transfer_queue.end_command_buffer();
}

void RenderContext::create_meshlet_buffer()
{
	// Create meshdraw, meshinfo, meshlet buffer
	auto physical_device = core_->get_physical_device();
	auto logical_device  = core_->get_logical_device();

	// Create a transfer queue for uploading data
	lz::ExecuteOnceQueue transfer_queue(core_);
	auto                 transfer_command_buffer = transfer_queue.begin_command_buffer();

	// Create a meshlet buffer
	mesh_let_buffer_ = std::make_unique<lz::StagedBuffer>(
	    physical_device, logical_device,
	    meshlets_.size() * sizeof(Meshlet),
	    vk::BufferUsageFlagBits::eStorageBuffer);

	if (!meshlets_.empty())
	{
		memcpy(mesh_let_buffer_->map(), meshlets_.data(), meshlets_.size() * sizeof(Meshlet));
		mesh_let_buffer_->unmap(transfer_command_buffer);
	}

	// Create a meshlet data buffer
	mesh_let_data_buffer_ = std::make_unique<lz::StagedBuffer>(
	    physical_device, logical_device,
	    meshlet_data_datum_.size() * sizeof(uint32_t),
	    vk::BufferUsageFlagBits::eStorageBuffer);

	if (!meshlet_data_datum_.empty())
	{
		memcpy(mesh_let_data_buffer_->map(), meshlet_data_datum_.data(), meshlet_data_datum_.size() * sizeof(uint32_t));
		mesh_let_data_buffer_->unmap(transfer_command_buffer);
	}

	// Submit the transfer command
	transfer_queue.end_command_buffer();
}

}        // namespace lz::render