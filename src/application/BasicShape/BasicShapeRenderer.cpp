#include "BasicShapeRenderer.h"

#include "backend/Core.h"


namespace lz::render
{
	BasicShapeRenderer::BasicShapeRenderer(lz::Core* core)
		: core_(core)
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
			glm::uvec2 size = {viewport_extent_.width, viewport_extent_.height};
			frame_resource.reset(new FrameResource(render_graph, size));
		}

		render_graph->add_pass(lz::RenderGraph::RenderPassDesc()
                       .set_color_attachments({
	                       {frame_info.swapchain_image_view_proxy_id, vk::AttachmentLoadOp::eClear}})
                       .set_depth_attachment(frame_resource->depth_stencil_proxy_.image_view_proxy.get().id(),
                                             vk::AttachmentLoadOp::eClear)
                       .set_render_area_extent(viewport_extent_)
                       .set_record_func([&](lz::RenderGraph::RenderPassContext context)
                       {
	                       const auto shader_program = base_shape_shader_.shader_program.get();
	                       const auto pipeline_info = 
							   this->core_->get_pipeline_cache()->bind_graphics_pipeline(
			                       context.get_command_buffer(),
			                       context.get_render_pass()->get_handle(),
			                       lz::DepthSettings::enabled(),
			                       {lz::BlendSettings::opaque()},
			                       vertex_decl_,
			                       vk::PrimitiveTopology::eTriangleList,
			                       shader_program);

						   // set = 0 uniform buffer binding
	                       const lz::DescriptorSetLayoutKey* shader_data_set_info = shader_program->get_set_info(k_shader_data_set_index);
	                       auto shader_data = frame_info.memory_pool->begin_set(shader_data_set_info);
	                       {
							   float aspect = static_cast<float>(viewport_extent_.width) / static_cast<float>(viewport_extent_.height);

		                       auto shader_data_buffer = frame_info.memory_pool->get_uniform_buffer_data<BasicShapeShader::DataBuffer>("ubo_data");// same name from shader
		                       shader_data_buffer->view_matrix = glm::inverse(camera.get_transform_matrix());
		                       shader_data_buffer->proj_matrix = glm::perspectiveZO(1.0f, aspect, 0.01f, 1000.0f)* glm::scale(glm::vec3(1.0f, -1.0f, -1.0f));

	                       }
	                       frame_info.memory_pool->end_set();

	                       auto shader_data_set = core_->get_descriptor_set_cache()->get_descriptor_set(*shader_data_set_info, shader_data.uniform_buffer_bindings, {}, {});

						   

						   const auto draw_call_set_info = shader_program->get_set_info(k_drawcall_data_set_index);

	                       scene->iterate_objects([&](glm::mat4 object_to_world, glm::vec3 albedo_color,
	                                                  glm::vec3 emissive_color, vk::Buffer vertex_buffer,
	                                                  vk::Buffer index_buffer, uint32_t vertices_count,
	                                                  uint32_t indices_count)
	                       {
								   auto draw_call_data_offset = frame_info.memory_pool->begin_set(draw_call_set_info);
								   {
									   auto draw_call_data = frame_info.memory_pool->get_uniform_buffer_data<DrawCallDataBuffer>("draw_call_data");
									   draw_call_data->model_matrix = object_to_world;
								   }
								   frame_info.memory_pool->end_set();
								   auto draw_call_set = core_->get_descriptor_set_cache()->get_descriptor_set(*draw_call_set_info, draw_call_data_offset.uniform_buffer_bindings, {}, {});

								   // bind shader data set

							   context.get_command_buffer().bindDescriptorSets(
								   vk::PipelineBindPoint::eGraphics, pipeline_info.pipeline_layout,
								   k_shader_data_set_index, { shader_data_set,draw_call_set }, { shader_data.dynamic_offset,draw_call_data_offset.dynamic_offset });
	                       
		                       context.get_command_buffer().bindVertexBuffers(0, {vertex_buffer}, {0});
		                       context.get_command_buffer().bindIndexBuffer(
			                       index_buffer, 0, vk::IndexType::eUint32);
		                       context.get_command_buffer().drawIndexed(indices_count, 1, 0, 0, 0);
	                       });
                       }));
	}

	void BasicShapeRenderer::reload_shaders()
	{
		base_shape_shader_.vertex_shader.reset(new lz::Shader(core_->get_logical_device(),SHADER_SPIRV_DIR"BasicShape/BasicShape.vert.spv"));
		base_shape_shader_.fragment_shader.reset(new lz::Shader(core_->get_logical_device(),SHADER_SPIRV_DIR"BasicShape/BasicShape.frag.spv"));
		base_shape_shader_.shader_program.reset(new lz::ShaderProgram({ base_shape_shader_.vertex_shader.get(), base_shape_shader_.fragment_shader.get() }));
	}

	void BasicShapeRenderer::change_view()
	{
	}
}
