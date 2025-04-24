#include "BasicShapeRenderer.h"

#include "backend/Core.h"


namespace lz::render
{
	BasicShapeRenderer::BasicShapeRenderer(lz::Core* core)
		:core_(core)
	{
		pipeline_cache_.reset(new lz::PipelineCache(core_->get_logical_device(), nullptr));

		vertex_decl_ = Mesh::get_vertex_declaration();

		reload_shaders();
	}

	void BasicShapeRenderer::recreate_scene_resources(lz::Scene* scene)
	{
		// we don't need to recreate scene resources for this renderer
	}

	void BasicShapeRenderer::recreate_swapchain_resources(vk::Extent2D viewport_extent, size_t in_flight_frames_count)
	{
		this->viewport_extent_ = viewport_extent;
		frame_resource_datum_.clear();
	}

	void BasicShapeRenderer::render_frame(const lz::InFlightQueue::FrameInfo& frame_info, const lz::Camera& camera,
		const lz::Camera& light, lz::Scene* scene, GLFWwindow* window)
	{
		auto render_graph = core_->get_render_graph();
		auto& frame_resource = frame_resource_datum_[render_graph];
		if (!frame_resource)
		{
			glm::uvec2 size = { viewport_extent_.width, viewport_extent_.height };
			frame_resource.reset(new FrameResource(render_graph, size));
		}

		vk::ClearValue clear_value;
		clear_value.color = { 0.0f, 0.0f, 0.0f, 1.0f };
		render_graph->add_pass(lz::RenderGraph::RenderPassDesc()
			.set_color_attachments({ {frame_info.swapchain_image_view_proxy_id,vk::AttachmentLoadOp::eClear,clear_value} })
			.set_depth_attachment(frame_resource->depth_stencil_proxy_.image_view_proxy.get().id(),vk::AttachmentLoadOp::eClear)
			.set_render_area_extent(viewport_extent_)
			.set_record_func([&](lz::RenderGraph::RenderPassContext context)
				{
					auto shader_program = base_shape_shader_.shader_program.get();
					auto pipeline_info = this->core_->get_pipeline_cache()->bind_graphics_pipeline(
						context.get_command_buffer(),
						context.get_render_pass()->get_handle(),
						lz::DepthSettings::enabled(),
						{ lz::BlendSettings::opaque() },
						vertex_decl_,
						vk::PrimitiveTopology::eTriangleList,
						shader_program
					);

					scene->iterate_objects([&](glm::mat4 object_to_world, glm::vec3 albedo_color,
						glm::vec3 emissive_color, vk::Buffer vertex_buffer,
						vk::Buffer index_buffer, uint32_t vertices_count,
						uint32_t indices_count)
						{
							context.get_command_buffer().bindVertexBuffers(0, { vertex_buffer }, {0});
							context.get_command_buffer().bindIndexBuffer(index_buffer, 0, vk::IndexType::eUint32);
							context.get_command_buffer().drawIndexed(indices_count, 1, 0, 0, 0);
						});
					
				}));
	}

	void BasicShapeRenderer::reload_shaders()
	{
		base_shape_shader_.vertex_shader.reset(new lz::Shader(core_->get_logical_device(), std::string(SHADER_SPIRV_DIR) + "BasicShape/BasicShape.vert.spv"));
		base_shape_shader_.fragment_shader.reset(new lz::Shader(core_->get_logical_device(), std::string(SHADER_SPIRV_DIR) + "BasicShape/BasicShape.frag.spv"));
		base_shape_shader_.shader_program.reset(new lz::ShaderProgram(base_shape_shader_.vertex_shader.get(), base_shape_shader_.fragment_shader.get()));
	}

	void BasicShapeRenderer::change_view()
	{
	}
}
