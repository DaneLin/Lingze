#include "SimpleMeshShadingRenderer.h"

#include "backend/Core.h"

namespace lz::render
{
SimpleMeshShadingRenderer::SimpleMeshShadingRenderer(lz::Core *core) :
    core_(core)
{
	reload_shaders();
}

void SimpleMeshShadingRenderer::recreate_scene_resources(lz::Scene *scene)
{
}

void SimpleMeshShadingRenderer::recreate_swapchain_resources(vk::Extent2D viewport_extent,
                                                             size_t       in_flight_frames_count)
{
	viewport_extent_ = viewport_extent;
}

void SimpleMeshShadingRenderer::render_frame(const lz::InFlightQueue::FrameInfo &frame_info,
                                             const lz::Camera &camera, const lz::Camera &light, lz::Scene *scene, GLFWwindow *window)
{
	auto render_graph = core_->get_render_graph();
	render_graph->add_pass(lz::RenderGraph::RenderPassDesc()
	                           .set_color_attachments({{frame_info.swapchain_image_view_proxy_id, vk::AttachmentLoadOp::eClear}})
	                           .set_render_area_extent(viewport_extent_)
	                           .set_record_func([this](lz::RenderGraph::RenderPassContext context) {
		                           auto shader_program = shader_program_.get();
		                           auto pipeline_info  = this->core_->get_pipeline_cache()->bind_graphics_pipeline(
                                       context.get_command_buffer(),
                                       context.get_render_pass()->get_handle(),
                                       lz::DepthSettings::disabled(),
                                       {lz::BlendSettings::opaque()},
                                       lz::VertexDeclaration(),
                                       vk::PrimitiveTopology::eTriangleList,
                                       shader_program);

		                           context.get_command_buffer().drawMeshTasksEXT(1, 1, 1, core_->get_dynamic_loader());
	                           }));
}

void SimpleMeshShadingRenderer::reload_shaders()
{
	task_shader_.reset(new Shader(core_->get_logical_device(), SHADER_GLSL_DIR "MeshShading/ms.task"));
	mesh_shader_.reset(new Shader(core_->get_logical_device(), SHADER_GLSL_DIR "MeshShading/ms.mesh"));
	fragment_shader_.reset(new Shader(core_->get_logical_device(), SHADER_GLSL_DIR "MeshShading/ps.frag"));
	shader_program_.reset(new ShaderProgram({task_shader_.get(), mesh_shader_.get(), fragment_shader_.get()}));
}

void SimpleMeshShadingRenderer::change_view()
{
}
}        // namespace lz::render
