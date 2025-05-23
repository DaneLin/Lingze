#include "GpuDrivenRenderer.h"

#include "backend/Core.h"
#include "backend/Logging.h"

namespace lz::render
{
GpuDrivenRenderer::GpuDrivenRenderer(lz::Core *core) :
    core_(core)
{
	reload_shaders();
}

void GpuDrivenRenderer::recreate_render_context_resources(lz::render::RenderContext *render_context)
{
	scene_resource_.reset(new SceneResource(core_, render_context));
}

void GpuDrivenRenderer::recreate_swapchain_resources(vk::Extent2D viewport_extent, size_t in_flight_frames_count)
{
	viewport_extent_ = viewport_extent;
	viewport_resource_datum_.clear();
}

void GpuDrivenRenderer::render_frame(const lz::InFlightQueue::FrameInfo &frame_info, const lz::Scene &scene, lz::render::RenderContext &render_context, GLFWwindow *window)
{
	auto  render_graph   = core_->get_render_graph();
	auto &frame_resource = viewport_resource_datum_[render_graph];
	if (!frame_resource)
	{
		glm::uvec2 size = {viewport_extent_.width, viewport_extent_.height};
		frame_resource.reset(new ViewportResource(render_graph, size));
	}

	struct alignas(4) CullData
	{
		glm::mat4 view_matrix;
		float     P00, P11, znear, zfar;        // symmetirc projection parameters
		float     frustum[4];                   // data for left / right / top / bottom
		uint32_t  draw_count;                   // number of draw commands
	};

	// pass 1 : culling
	render_graph->add_pass(
	    lz::RenderGraph::ComputePassDesc()
	        .set_storage_buffers({scene_resource_->visible_mesh_draw_proxy_.get().id(), scene_resource_->visible_mesh_count_proxy_.get().id()})
	        .set_profiler_info(lz::Colors::carrot, "CullingPass")
	        .set_record_func([&](lz::RenderGraph::PassContext context) {
		        auto pipeline_info = core_->get_pipeline_cache()->bind_compute_pipeline(context.get_command_buffer(), culling_shader_.compute_shader.get());

		        const lz::DescriptorSetLayoutKey *shader_data_set_info = culling_shader_.compute_shader->get_set_info(k_shader_data_set_index);

		        // uniform data
		        auto shader_data = frame_info.memory_pool->begin_set(shader_data_set_info);
		        {
			        auto      main_camera   = scene.get_main_camera();
			        glm::mat4 proj_matrix   = main_camera->get_projection_matrix();
			        glm::mat4 proj_matrix_t = glm::transpose(proj_matrix);
			        glm::vec4 frustum_x     = glm::normalize(proj_matrix_t[3] + proj_matrix_t[0]);
			        glm::vec4 frustum_y     = glm::normalize(proj_matrix_t[3] + proj_matrix_t[1]);

			        auto cull_data         = frame_info.memory_pool->get_uniform_buffer_data<CullData>("UboData");
			        cull_data->view_matrix = glm::inverse(main_camera->get_transform_matrix());
			        cull_data->P00         = proj_matrix[0][0];
			        cull_data->P11         = proj_matrix[1][1];
			        cull_data->znear       = main_camera->get_near_plane();
			        cull_data->zfar        = main_camera->get_far_plane();
			        cull_data->frustum[0]  = frustum_x.x;
			        cull_data->frustum[1]  = frustum_x.z;
			        cull_data->frustum[2]  = frustum_y.y;
			        cull_data->frustum[3]  = frustum_y.z;
			        cull_data->draw_count  = uint32_t(render_context.get_draw_count());
		        }

		        frame_info.memory_pool->end_set();

		        std::vector<lz::StorageBufferBinding> storage_buffer_bindings;
		        storage_buffer_bindings.push_back(shader_data_set_info->make_storage_buffer_binding("MeshData", &render_context.get_mesh_info_buffer()));
		        storage_buffer_bindings.push_back(shader_data_set_info->make_storage_buffer_binding("MeshDrawData", &render_context.get_mesh_draw_buffer()));
		        auto visible_mesh_draw_proxy = context.get_buffer(scene_resource_->visible_mesh_draw_proxy_.get().id());
		        storage_buffer_bindings.push_back(shader_data_set_info->make_storage_buffer_binding("VisibleMeshDrawCommand", visible_mesh_draw_proxy));
		        auto visible_mesh_count_proxy = context.get_buffer(scene_resource_->visible_mesh_count_proxy_.get().id());
		        storage_buffer_bindings.push_back(shader_data_set_info->make_storage_buffer_binding("VisibleMeshDrawCommandCount", visible_mesh_count_proxy));

		        auto shader_data_set = core_->get_descriptor_set_cache()->get_descriptor_set(*shader_data_set_info, shader_data.uniform_buffer_bindings, storage_buffer_bindings, {});

		        // Clear visible mesh count buffer
		        context.get_command_buffer().fillBuffer(
		            visible_mesh_count_proxy->get_handle(), 0, sizeof(uint32_t), 0);

		        vk::BufferMemoryBarrier clear_barrier = vk::BufferMemoryBarrier()
		                                                    .setBuffer(visible_mesh_count_proxy->get_handle())
		                                                    .setSize(sizeof(uint32_t))
		                                                    .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
		                                                    .setDstAccessMask(vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite);
		        context.get_command_buffer().pipelineBarrier(
		            vk::PipelineStageFlagBits::eTransfer,
		            vk::PipelineStageFlagBits::eComputeShader,
		            vk::DependencyFlags(),
		            {},
		            {clear_barrier},
		            {});

		        context.get_command_buffer().bindDescriptorSets(
		            vk::PipelineBindPoint::eCompute,
		            pipeline_info.pipeline_layout, k_shader_data_set_index,
		            {shader_data_set}, {shader_data.dynamic_offset});

		        uint32_t dispatch_x = uint32_t((render_context.get_draw_count() + 31) / 32);
		        context.get_command_buffer().dispatch(dispatch_x, 1, 1);

		        vk::BufferMemoryBarrier fill_barrier = vk::BufferMemoryBarrier()
		                                                   .setBuffer(visible_mesh_count_proxy->get_handle())
		                                                   .setSize(sizeof(uint32_t))
		                                                   .setSrcAccessMask(vk::AccessFlagBits::eShaderWrite)
		                                                   .setDstAccessMask(vk::AccessFlagBits::eIndirectCommandRead);
		        context.get_command_buffer().pipelineBarrier(
		            vk::PipelineStageFlagBits::eComputeShader,
		            vk::PipelineStageFlagBits::eDrawIndirect,
		            vk::DependencyFlags(),
		            {},
		            {fill_barrier},
		            {});
	        }));

	// pass 2 : rendering
	render_graph->add_pass(
	    lz::RenderGraph::RenderPassDesc()
	        .set_color_attachments({{frame_info.swapchain_image_view_proxy_id, vk::AttachmentLoadOp::eClear}})
	        .set_depth_attachment(
	            frame_resource->depth_stencil_proxy_.image_view_proxy.get().id(),
	            vk::AttachmentLoadOp::eClear)
	        .set_render_area_extent(viewport_extent_)
	        .set_storage_buffers({scene_resource_->visible_mesh_draw_proxy_.get().id(), scene_resource_->visible_mesh_count_proxy_.get().id()})
	        .set_profiler_info(lz::Colors::nephritis, "RenderingPass")
	        .set_record_func([&](lz::RenderGraph::RenderPassContext context) {
		        auto visible_mesh_draw_proxy  = context.get_buffer(scene_resource_->visible_mesh_draw_proxy_.get().id());
		        auto visible_mesh_count_proxy = context.get_buffer(scene_resource_->visible_mesh_count_proxy_.get().id());

		        auto shader_program = base_shape_shader_.shader_program.get();
		        auto pipeline_info  = core_->get_pipeline_cache()->bind_graphics_pipeline(
                    context.get_command_buffer(),
                    context.get_render_pass()->get_handle(),
                    lz::DepthSettings::enabled(),
                    {lz::BlendSettings::opaque()}, lz::VertexDeclaration(),
                    vk::PrimitiveTopology::eTriangleList, shader_program);

		        // set = 0 uniform buffer binding
		        const lz::DescriptorSetLayoutKey *shader_data_set_info = shader_program->get_set_info(k_shader_data_set_index);
		        // for uniform data
		        auto shader_data =
		            frame_info.memory_pool->begin_set(shader_data_set_info);
		        {
			        auto main_camera                = scene.get_main_camera();
			        auto shader_data_buffer         = frame_info.memory_pool->get_uniform_buffer_data<BasicShapeShader::DataBuffer>("GlobalData");
			        shader_data_buffer->view_matrix = main_camera->get_view_matrix();
			        shader_data_buffer->proj_matrix = main_camera->get_projection_matrix();
		        }
		        frame_info.memory_pool->end_set();

		        // create storage binding
		        std::vector<lz::StorageBufferBinding> storage_buffer_bindings;
		        storage_buffer_bindings.push_back(shader_data_set_info->make_storage_buffer_binding("VertexBuffer", &render_context.get_global_vertex_buffer()));
		        storage_buffer_bindings.push_back(shader_data_set_info->make_storage_buffer_binding("VisibleMeshDrawCommand", visible_mesh_draw_proxy));
		        storage_buffer_bindings.push_back(shader_data_set_info->make_storage_buffer_binding("MeshDrawData", &render_context.get_mesh_draw_buffer()));
		        auto shader_data_set =
		            core_->get_descriptor_set_cache()->get_descriptor_set(
		                *shader_data_set_info,
		                shader_data.uniform_buffer_bindings,
		                storage_buffer_bindings, {});

		        context.get_command_buffer().bindDescriptorSets(
		            vk::PipelineBindPoint::eGraphics,
		            pipeline_info.pipeline_layout, k_shader_data_set_index,
		            {shader_data_set}, {shader_data.dynamic_offset});

		        context.get_command_buffer().bindIndexBuffer(render_context.get_global_index_buffer().get_handle(), 0, vk::IndexType::eUint32);
		        context.get_command_buffer().drawIndexedIndirectCount(
		            visible_mesh_draw_proxy->get_handle(), 0,
		            visible_mesh_count_proxy->get_handle(), 0,
		            static_cast<uint32_t>(render_context.get_draw_count()),
		            sizeof(lz::render::DrawCommand));
	        }));
}

void GpuDrivenRenderer::reload_shaders()
{
	const auto &logical_device = core_->get_logical_device();

	base_shape_shader_.vertex_shader.reset(new Shader(logical_device, SHADER_GLSL_DIR "GpuDriven/BasicShape.vert"));
	base_shape_shader_.fragment_shader.reset(new Shader(logical_device, SHADER_GLSL_DIR "GpuDriven/BasicShape.frag"));
	base_shape_shader_.shader_program.reset(new ShaderProgram({base_shape_shader_.vertex_shader.get(), base_shape_shader_.fragment_shader.get()}));

	culling_shader_.compute_shader.reset(new Shader(logical_device, SHADER_GLSL_DIR "GpuDriven/Culling.comp"));
}

void GpuDrivenRenderer::change_view()
{}

GpuDrivenRenderer::SceneResource::SceneResource(lz::Core *core, lz::render::RenderContext *render_context)
{
	visible_mesh_draw_proxy_  = core->get_render_graph()->add_buffer<lz::render::DrawCommand>(uint32_t(render_context->get_draw_count()));
	visible_mesh_count_proxy_ = core->get_render_graph()->add_buffer<uint32_t>(1);
	mesh_draw_proxy_          = core->get_render_graph()->add_external_buffer(&render_context->get_mesh_draw_buffer());
	mesh_proxy_               = core->get_render_graph()->add_external_buffer(&render_context->get_mesh_info_buffer());
}
}        // namespace lz::render
