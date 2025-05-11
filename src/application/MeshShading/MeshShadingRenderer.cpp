#include "MeshShadingRenderer.h"

#include "backend/Core.h"

namespace lz::render
{
MeshShadingRenderer::MeshShadingRenderer(lz::Core *core) :
    core_(core)
{
	reload_shaders();
}

void MeshShadingRenderer::recreate_swapchain_resources(vk::Extent2D viewport_extent, size_t in_flight_frames_count)
{
	viewport_extent_ = viewport_extent;
	frame_resource_datum_.clear();
}

void MeshShadingRenderer::render_frame(
    const lz::InFlightQueue::FrameInfo &frame_info, const lz::Scene &scene,
    lz::render::RenderContext &render_context, GLFWwindow *window)
{
	auto  render_graph   = core_->get_render_graph();
	auto &frame_resource = frame_resource_datum_[render_graph];
	if (!frame_resource)
	{
		glm::uvec2 size = {viewport_extent_.width, viewport_extent_.height};
		frame_resource.reset(new FrameResource(render_graph, size));
	}
	render_graph->add_pass(
	    lz::RenderGraph::RenderPassDesc()
	        .set_color_attachments({{frame_info.swapchain_image_view_proxy_id, vk::AttachmentLoadOp::eClear}})
	        .set_depth_attachment(
	            frame_resource->depth_stencil_proxy_.image_view_proxy.get().id(),
	            vk::AttachmentLoadOp::eClear)
	        .set_render_area_extent(viewport_extent_)
	        .set_record_func([&](lz::RenderGraph::RenderPassContext context) {
		        if (core_->mesh_shader_supported())
		        {
			        auto shader_program = meshlet_shader_.shader_program.get();
			        auto pipeline_info =
			            core_->get_pipeline_cache()->bind_graphics_pipeline(
			                context.get_command_buffer(),
			                context.get_render_pass()->get_handle(),
			                lz::DepthSettings::enabled(),
			                {lz::BlendSettings::opaque()}, lz::VertexDeclaration(),
			                vk::PrimitiveTopology::eTriangleList, shader_program);

			        // set = 0 uniform buffer binding
			        const lz::DescriptorSetLayoutKey *shader_data_set_info = shader_program->get_set_info(k_shader_data_set_index);
			        // for uniform data
			        auto shader_data = frame_info.memory_pool->begin_set(shader_data_set_info);
			        {
				        auto shader_data_buffer = frame_info.memory_pool->get_uniform_buffer_data<DataBuffer>("ubo_data");
				        auto main_camera        = scene.get_main_camera();
				        // same name from shader
				        shader_data_buffer->view_matrix = main_camera->get_view_matrix();
				        shader_data_buffer->proj_matrix = main_camera->get_projection_matrix();
			        }
			        frame_info.memory_pool->end_set();

			        // create storage binding
			        std::vector<lz::StorageBufferBinding> storage_buffer_bindings;
			        storage_buffer_bindings.push_back(shader_data_set_info->make_storage_buffer_binding("Vertices", &render_context.get_global_vertex_buffer()));
			        storage_buffer_bindings.push_back(shader_data_set_info->make_storage_buffer_binding("Meshlets", &render_context.get_mesh_let_buffer()));
			        storage_buffer_bindings.push_back(shader_data_set_info->make_storage_buffer_binding("MeshletDataBuffer", &render_context.get_mesh_let_data_buffer()));

			        // TODO: add draw call buffer for model matrix
			        auto shader_data_set =
			            core_->get_descriptor_set_cache()->get_descriptor_set(
			                *shader_data_set_info,
			                shader_data.uniform_buffer_bindings,
			                storage_buffer_bindings, {});

			        context.get_command_buffer().bindDescriptorSets(
			            vk::PipelineBindPoint::eGraphics,
			            pipeline_info.pipeline_layout, k_shader_data_set_index,
			            {shader_data_set}, {shader_data.dynamic_offset});

			        uint32_t task_need_count = uint32_t(render_context.get_meshlet_count() / 32);
			        context.get_command_buffer().drawMeshTasksEXT(
			            task_need_count, 1, 1, core_->get_dynamic_loader());
		        }
	        }));
}

void MeshShadingRenderer::reload_shaders()
{
	const auto &logical_device = core_->get_logical_device();

	meshlet_shader_.task_shader.reset(new Shader(logical_device, SHADER_GLSL_DIR "MeshShading/meshlet.task"));
	meshlet_shader_.mesh_shader.reset(new Shader(logical_device, SHADER_GLSL_DIR "MeshShading/meshlet.mesh"));
	meshlet_shader_.fragment_shader.reset(new Shader(logical_device, SHADER_GLSL_DIR "MeshShading/meshlet.frag"));
	meshlet_shader_.shader_program.reset(new ShaderProgram({meshlet_shader_.task_shader.get(), meshlet_shader_.mesh_shader.get(), meshlet_shader_.fragment_shader.get()}));
}

void MeshShadingRenderer::change_view()
{}
}        // namespace lz::render
