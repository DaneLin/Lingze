#include "MeshShadingRenderer.h"

#include "backend/Core.h"

#include "backend/EngineConfig.h"
#include "backend/MathUtils.h"

namespace lz::render
{
MeshShadingRenderer::MeshShadingRenderer(lz::Core *core) :
    core_(core)
{
	depth_reduce_sampler_ = std::make_unique<Sampler>(core_->get_logical_device(), vk::SamplerAddressMode::eClampToEdge, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear, vk::SamplerReductionModeEXT::eMax);

	reload_shaders();
}

void MeshShadingRenderer::recreate_swapchain_resources(vk::Extent2D viewport_extent, size_t in_flight_frames_count)
{
	viewport_extent_ = viewport_extent;
	frame_resource_datum_.clear();
}

void MeshShadingRenderer::recreate_render_context_resources(lz::render::RenderContext *render_context)
{
	scene_resource_.reset(new SceneResource(core_, render_context));
}

void MeshShadingRenderer::generate_depth_pyramid(const lz::InFlightQueue::FrameInfo &frame_info, const lz::Scene &scene, lz::render::RenderContext &render_context, lz::RenderGraph *render_graph, UnmippedImageProxy &depth_stencil_proxy, MippedImageProxy &depth_pyramid_proxy)
{
	uint32_t mip_width  = depth_pyramid_proxy.base_size.x;
	uint32_t mip_height = depth_pyramid_proxy.base_size.y;

	struct PushData
	{
		glm::vec2 image_size;
	};

	for (size_t mip_index = 0; mip_index < depth_pyramid_proxy.mip_image_view_proxies.size(); ++mip_index)
	{
		auto dst_proxy_id = depth_pyramid_proxy.mip_image_view_proxies[mip_index]->id();
		auto src_proxy_id = mip_index == 0 ? depth_stencil_proxy.image_view_proxy->id() : depth_pyramid_proxy.mip_image_view_proxies[mip_index - 1]->id();
		render_graph->add_pass(
		    lz::RenderGraph::ComputePassDesc()
		        .set_input_images({src_proxy_id})
		        .set_storage_images({dst_proxy_id})
		        .set_profiler_info(lz::Colors::carrot, "DepthPyramidPass")
		        .set_record_func([this, frame_info, mip_width, mip_height, mip_index, src_proxy_id, dst_proxy_id](lz::RenderGraph::PassContext context) {
			        auto pipeline_info = core_->get_pipeline_cache()->bind_compute_pipeline(context.get_command_buffer(), depth_pyramid_shader_.compute_shader.get());

			        const lz::DescriptorSetLayoutKey *shader_data_set_info = depth_pyramid_shader_.compute_shader->get_set_info(k_shader_data_set_index);

			        uint32_t level_width  = std::max(1u, mip_width >> mip_index);
			        uint32_t level_height = std::max(1u, mip_height >> mip_index);
			        auto     shader_data  = frame_info.memory_pool->begin_set(shader_data_set_info);
			        {
				        auto push_data        = frame_info.memory_pool->get_uniform_buffer_data<PushData>("ImageData");
				        push_data->image_size = {float(level_width), float(level_height)};
			        }
			        frame_info.memory_pool->end_set();

			        std::vector<lz::ImageSamplerBinding> image_sampler_bindings;

			        auto depth_image_view         = context.get_image_view(src_proxy_id);
			        auto depth_pyramid_image_view = context.get_image_view(dst_proxy_id);
			        image_sampler_bindings.push_back(shader_data_set_info->make_image_sampler_binding("in_image", depth_image_view, depth_reduce_sampler_.get()));

			        std::vector<lz::StorageImageBinding> storage_image_sampler_bindings;
			        storage_image_sampler_bindings.push_back(shader_data_set_info->make_storage_image_binding("out_image", depth_pyramid_image_view));

			        auto shader_data_set = core_->get_descriptor_set_cache()->get_descriptor_set(*shader_data_set_info, shader_data.uniform_buffer_bindings, {}, storage_image_sampler_bindings, image_sampler_bindings);

			        // TODO: Support push constant
			        /*PushData push_data;
			        push_data.image_size = {mip_width, mip_height};
			        context.get_command_buffer().pushConstants(pipeline_info.pipeline_layout,
			                                                   vk::ShaderStageFlagBits::eCompute,
			                                                   0,
			                                                   sizeof(PushData),
			                                                   &push_data);*/

			        context.get_command_buffer().bindDescriptorSets(
			            vk::PipelineBindPoint::eCompute,
			            pipeline_info.pipeline_layout, k_shader_data_set_index,
			            {shader_data_set}, {shader_data.dynamic_offset});

			        context.get_command_buffer().dispatch(lz::math::get_group_count(level_width, COMPUTE_WGSIZE), lz::math::get_group_count(level_height, COMPUTE_WGSIZE), 1);
		        }));
	}
}

void MeshShadingRenderer::cull_last_frame_visible(const lz::InFlightQueue::FrameInfo &frame_info, const lz::Scene &scene, lz::render::RenderContext &render_context, lz::RenderGraph *render_graph)
{
	render_graph->add_pass(
	    lz::RenderGraph::TransferPassDesc()
	        .set_dst_buffers({scene_resource_->visible_meshtask_count_proxy_.get().id()})
	        .set_profiler_info(lz::Colors::carrot, "ClearVisibleMeshTaskPass")
	        .set_record_func([&](lz::RenderGraph::PassContext context) {
		        auto visible_meshtask_count_proxy = context.get_buffer(scene_resource_->visible_meshtask_count_proxy_.get().id());
		        context.get_command_buffer().fillBuffer(visible_meshtask_count_proxy->get_handle(), 0, sizeof(uint32_t), 0);
	        }));

	// pass 1 : culling
	render_graph->add_pass(
	    lz::RenderGraph::ComputePassDesc()
	        .set_storage_buffers({scene_resource_->mesh_proxy_.get().id(), scene_resource_->mesh_draw_proxy_.get().id(), scene_resource_->draw_visibility_buffer_proxy_.get().id(), scene_resource_->visible_meshtask_draw_proxy_.get().id()})
	        .set_indirect_buffers({scene_resource_->visible_meshtask_count_proxy_.get().id()})
	        .set_profiler_info(lz::Colors::carrot, "DrawCullPass")
	        .set_record_func([&](lz::RenderGraph::PassContext context) {
		        auto pipeline_info = core_->get_pipeline_cache()->bind_compute_pipeline(context.get_command_buffer(), draw_cull_shader_.compute_shader.get());

		        const lz::DescriptorSetLayoutKey *shader_data_set_info = draw_cull_shader_.compute_shader->get_set_info(k_shader_data_set_index);

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
		        auto visible_meshtask_draw_proxy = context.get_buffer(scene_resource_->visible_meshtask_draw_proxy_.get().id());
		        storage_buffer_bindings.push_back(shader_data_set_info->make_storage_buffer_binding("VisibleMeshTaskDrawCommand", visible_meshtask_draw_proxy));
		        auto visible_meshtask_count_proxy = context.get_buffer(scene_resource_->visible_meshtask_count_proxy_.get().id());
		        storage_buffer_bindings.push_back(shader_data_set_info->make_storage_buffer_binding("VisibleMeshTaskDrawCommandCount", visible_meshtask_count_proxy));
		        auto draw_visibility_buffer_proxy = context.get_buffer(scene_resource_->draw_visibility_buffer_proxy_.get().id());
		        storage_buffer_bindings.push_back(shader_data_set_info->make_storage_buffer_binding("DrawVisibilityBuffer", draw_visibility_buffer_proxy));

		        auto shader_data_set = core_->get_descriptor_set_cache()->get_descriptor_set(*shader_data_set_info, shader_data.uniform_buffer_bindings, storage_buffer_bindings, {});

		        context.get_command_buffer().bindDescriptorSets(
		            vk::PipelineBindPoint::eCompute,
		            pipeline_info.pipeline_layout, k_shader_data_set_index,
		            {shader_data_set}, {shader_data.dynamic_offset});

		        uint32_t dispatch_x = uint32_t((render_context.get_draw_count() + COMPUTE_WGSIZE - 1) / COMPUTE_WGSIZE);
		        context.get_command_buffer().dispatch(dispatch_x, 1, 1);
	        }));
}

void MeshShadingRenderer::cull_last_frame_not_visible(const lz::InFlightQueue::FrameInfo &frame_info, const lz::Scene &scene, lz::render::RenderContext &render_context, lz::RenderGraph *render_graph, MippedImageProxy &depth_pyramid_proxy)
{
	render_graph->add_pass(
	    lz::RenderGraph::TransferPassDesc()
	        .set_dst_buffers({scene_resource_->visible_meshtask_count_proxy_.get().id()})
	        .set_profiler_info(lz::Colors::carrot, "ClearVisibleMeshTaskPass2")
	        .set_record_func([&](lz::RenderGraph::PassContext context) {
		        auto visible_meshtask_count_proxy = context.get_buffer(scene_resource_->visible_meshtask_count_proxy_.get().id());
		        context.get_command_buffer().fillBuffer(visible_meshtask_count_proxy->get_handle(), 0, sizeof(uint32_t), 0);
	        }));

	// pass 1 : culling
	render_graph->add_pass(
	    lz::RenderGraph::ComputePassDesc()
	        .set_storage_buffers({scene_resource_->visible_meshtask_draw_proxy_.get().id(), scene_resource_->draw_visibility_buffer_proxy_.get().id()})
	        .set_indirect_buffers({scene_resource_->visible_meshtask_count_proxy_.get().id()})
			.set_input_images({depth_pyramid_proxy.image_view_proxy.get().id()})
	        .set_profiler_info(lz::Colors::carrot, "DrawCullPass")
	        .set_record_func([&](lz::RenderGraph::PassContext context) {
		        auto pipeline_info = core_->get_pipeline_cache()->bind_compute_pipeline(context.get_command_buffer(), draw_cull_late_shader_.compute_shader.get());

		        const lz::DescriptorSetLayoutKey *shader_data_set_info = draw_cull_late_shader_.compute_shader->get_set_info(k_shader_data_set_index);

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
			        cull_data->depth_pyramid_width = depth_pyramid_proxy.base_size.x;
			        cull_data->depth_pyramid_height = depth_pyramid_proxy.base_size.y;

		        }

		        frame_info.memory_pool->end_set();

		        std::vector<lz::StorageBufferBinding> storage_buffer_bindings;
		        storage_buffer_bindings.push_back(shader_data_set_info->make_storage_buffer_binding("MeshData", &render_context.get_mesh_info_buffer()));
		        storage_buffer_bindings.push_back(shader_data_set_info->make_storage_buffer_binding("MeshDrawData", &render_context.get_mesh_draw_buffer()));
		        auto visible_meshtask_draw_proxy = context.get_buffer(scene_resource_->visible_meshtask_draw_proxy_.get().id());
		        storage_buffer_bindings.push_back(shader_data_set_info->make_storage_buffer_binding("VisibleMeshTaskDrawCommand", visible_meshtask_draw_proxy));
		        auto visible_meshtask_count_proxy = context.get_buffer(scene_resource_->visible_meshtask_count_proxy_.get().id());
		        storage_buffer_bindings.push_back(shader_data_set_info->make_storage_buffer_binding("VisibleMeshTaskDrawCommandCount", visible_meshtask_count_proxy));
		        auto draw_visibility_buffer_proxy = context.get_buffer(scene_resource_->draw_visibility_buffer_proxy_.get().id());
		        storage_buffer_bindings.push_back(shader_data_set_info->make_storage_buffer_binding("DrawVisibilityBuffer", draw_visibility_buffer_proxy));

				std::vector<lz::ImageSamplerBinding> image_sampler_bindings;
		        auto                                 depth_pyramid_image_view = context.get_image_view(depth_pyramid_proxy.image_view_proxy.get().id());
		        image_sampler_bindings.push_back(shader_data_set_info->make_image_sampler_binding("depth_pyramid", depth_pyramid_image_view, depth_reduce_sampler_.get()));

		        auto shader_data_set = core_->get_descriptor_set_cache()->get_descriptor_set(*shader_data_set_info, shader_data.uniform_buffer_bindings, storage_buffer_bindings, {}, image_sampler_bindings);

		        context.get_command_buffer().bindDescriptorSets(
		            vk::PipelineBindPoint::eCompute,
		            pipeline_info.pipeline_layout, k_shader_data_set_index,
		            {shader_data_set}, {shader_data.dynamic_offset});

		        uint32_t dispatch_x = uint32_t((render_context.get_draw_count() + COMPUTE_WGSIZE - 1) / COMPUTE_WGSIZE);
		        context.get_command_buffer().dispatch(dispatch_x, 1, 1);
	        }));
}

void MeshShadingRenderer::draw_mesh_task(const lz::InFlightQueue::FrameInfo &frame_info, const lz::Scene &scene, lz::render::RenderContext &render_context, lz::RenderGraph *render_graph, UnmippedImageProxy &depth_stencil_proxy, bool late)
{
	struct alignas(4) DataBuffer
	{
		glm::mat4 view_matrix;
		glm::mat4 proj_matrix;
		float     screen_width;
		float     screen_height;
	};

	render_graph->add_pass(
	    lz::RenderGraph::RenderPassDesc()
	        .set_color_attachments({{frame_info.swapchain_image_view_proxy_id, late ? vk::AttachmentLoadOp::eLoad :vk::AttachmentLoadOp::eClear}})
	        .set_depth_attachment(depth_stencil_proxy.image_view_proxy.get().id(),late ? vk::AttachmentLoadOp::eLoad : vk::AttachmentLoadOp::eClear)
	        .set_storage_buffers({scene_resource_->mesh_proxy_.get().id(), scene_resource_->mesh_draw_proxy_.get().id(),scene_resource_->visible_meshtask_draw_proxy_.get().id()})
	        .set_indirect_buffers({ scene_resource_->visible_meshtask_count_proxy_.get().id()})
	        .set_render_area_extent(viewport_extent_)
	        .set_profiler_info(lz::Colors::peter_river, "MeshShadingPass")
	        .set_record_func([&](lz::RenderGraph::RenderPassContext context) {
		        if (core_->mesh_shader_supported())
		        {
			        auto shader_program = meshlet_shader_.shader_program.get();
			        auto pipeline_info =
			            core_->get_pipeline_cache()->bind_graphics_pipeline(
			                context.get_command_buffer(),
			                context.get_render_pass()->get_handle(),
			                lz::DepthSettings::enabled(),
			                {lz::BlendSettings::opaque()},
			                lz::VertexDeclaration(),
			                vk::PrimitiveTopology::eTriangleList, shader_program);

			        auto visible_meshtask_draw_proxy  = context.get_buffer(scene_resource_->visible_meshtask_draw_proxy_.get().id());
			        auto visible_meshtask_count_proxy = context.get_buffer(scene_resource_->visible_meshtask_count_proxy_.get().id());

			        // set = 0 uniform buffer binding
			        const lz::DescriptorSetLayoutKey *shader_data_set_info = shader_program->get_set_info(k_shader_data_set_index);

			        // for uniform data
			        auto shader_data = frame_info.memory_pool->begin_set(shader_data_set_info);
			        {
				        auto shader_data_buffer = frame_info.memory_pool->get_uniform_buffer_data<DataBuffer>("UboData");
				        auto main_camera        = scene.get_main_camera();
				        // same name from shader
				        shader_data_buffer->view_matrix   = main_camera->get_view_matrix();
				        shader_data_buffer->proj_matrix   = main_camera->get_projection_matrix();
				        shader_data_buffer->screen_width  = float(viewport_extent_.width);
				        shader_data_buffer->screen_height = float(viewport_extent_.height);
			        }
			        frame_info.memory_pool->end_set();
			        // create storage binding
			        std::vector<lz::StorageBufferBinding> storage_buffer_bindings;
			        storage_buffer_bindings.push_back(shader_data_set_info->make_storage_buffer_binding("Vertices", &render_context.get_global_vertex_buffer()));
			        storage_buffer_bindings.push_back(shader_data_set_info->make_storage_buffer_binding("Meshlets", &render_context.get_mesh_let_buffer()));
			        storage_buffer_bindings.push_back(shader_data_set_info->make_storage_buffer_binding("MeshletDataBuffer", &render_context.get_mesh_let_data_buffer()));
			        storage_buffer_bindings.push_back(shader_data_set_info->make_storage_buffer_binding("MaterialParametersBuffer", core_->get_material_parameters_buffer()));
			        storage_buffer_bindings.push_back(shader_data_set_info->make_storage_buffer_binding("VisibleMeshTaskDrawCommand", visible_meshtask_draw_proxy));
			        storage_buffer_bindings.push_back(shader_data_set_info->make_storage_buffer_binding("MeshDraws", &render_context.get_mesh_draw_buffer()));

			        auto shader_data_set = core_->get_descriptor_set_cache()->get_descriptor_set(
			            *shader_data_set_info,
			            shader_data.uniform_buffer_bindings,
			            storage_buffer_bindings, {});

			        context.get_command_buffer().bindDescriptorSets(
			            vk::PipelineBindPoint::eGraphics,
			            pipeline_info.pipeline_layout, k_shader_data_set_index,
			            {shader_data_set}, {shader_data.dynamic_offset});

			        // bind bindless descriptor set
			        context.get_command_buffer().bindDescriptorSets(
			            vk::PipelineBindPoint::eGraphics,
			            pipeline_info.pipeline_layout, BINDLESS_SET_ID,
			            {core_->get_bindless_descriptor_set()->get()}, {});

			        context.get_command_buffer().drawMeshTasksIndirectCountEXT(
			            visible_meshtask_draw_proxy->get_handle(),
			            0,
			            visible_meshtask_count_proxy->get_handle(),
			            0,
			            static_cast<uint32_t>(render_context.get_meshlet_count()),
			            sizeof(lz::render::MeshTaskDrawCommand),
			            core_->get_dynamic_loader());
		        }
	        }));
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
	cull_last_frame_visible(frame_info, scene, render_context, render_graph);
	draw_mesh_task(frame_info, scene, render_context, render_graph, frame_resource->depth_stencil_proxy, false);
	generate_depth_pyramid(frame_info, scene, render_context, render_graph, frame_resource->depth_stencil_proxy, frame_resource->depth_pyramid_proxy);
	cull_last_frame_not_visible(frame_info, scene, render_context, render_graph, frame_resource->depth_pyramid_proxy);
	draw_mesh_task(frame_info, scene, render_context, render_graph, frame_resource->depth_stencil_proxy, true);
}

void MeshShadingRenderer::reload_shaders()
{
	const auto &logical_device = core_->get_logical_device();

	draw_cull_shader_.compute_shader.reset(new Shader(logical_device, SHADER_GLSL_DIR "MeshShading/drawcull.comp"));
	draw_cull_late_shader_.compute_shader.reset(new Shader(logical_device, SHADER_GLSL_DIR "MeshShading/drawcull_late.comp"));
	depth_pyramid_shader_.compute_shader.reset(new Shader(logical_device, SHADER_GLSL_DIR "MeshShading/depthreduce.comp"));

	meshlet_shader_.task_shader.reset(new Shader(logical_device, SHADER_GLSL_DIR "MeshShading/meshlet.task"));
	meshlet_shader_.mesh_shader.reset(new Shader(logical_device, SHADER_GLSL_DIR "MeshShading/meshlet.mesh"));
	meshlet_shader_.fragment_shader.reset(new Shader(logical_device, SHADER_GLSL_DIR "MeshShading/meshlet.frag"));
	meshlet_shader_.shader_program.reset(new ShaderProgram({meshlet_shader_.task_shader.get(), meshlet_shader_.mesh_shader.get(), meshlet_shader_.fragment_shader.get()}));
}

void MeshShadingRenderer::change_view()
{}

MeshShadingRenderer::SceneResource::SceneResource(lz::Core *core, lz::render::RenderContext *render_context)
{
	visible_meshtask_draw_proxy_  = core->get_render_graph()->add_buffer<lz::render::MeshTaskDrawCommand>(uint32_t(render_context->get_meshlet_count()));
	visible_meshtask_count_proxy_ = core->get_render_graph()->add_buffer<uint32_t>(1);
	draw_visibility_buffer_proxy_ = core->get_render_graph()->add_buffer<uint32_t>(uint32_t(render_context->get_draw_count()));
	mesh_draw_proxy_              = core->get_render_graph()->add_external_buffer(&render_context->get_mesh_draw_buffer());
	mesh_proxy_                   = core->get_render_graph()->add_external_buffer(&render_context->get_mesh_info_buffer());
}

}        // namespace lz::render
