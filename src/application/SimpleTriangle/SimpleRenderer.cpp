#include "SimpleRenderer.h"

#include "backend/Core.h"


namespace lz::render
{
	SimpleRenderer::SimpleRenderer(lz::Core* core)
		: core_(core)
	{

		reload_shaders();
	}

	void SimpleRenderer::recreate_scene_resources(lz::Scene* scene) 
	{
		// we don't need to recreate scene resources for this renderer
	 }
	void SimpleRenderer::recreate_swapchain_resources(vk::Extent2D viewport_extent, size_t in_flight_frames_count) 
	{
		viewport_extent_ = viewport_extent;
	 }
	void SimpleRenderer::render_frame(const lz::InFlightQueue::FrameInfo& frame_info, const lz::Camera& camera, const lz::Camera& light, lz::Scene* scene, GLFWwindow* window) 
	{
		core_->get_render_graph()->add_pass(lz::RenderGraph::RenderPassDesc()
			.set_color_attachments({{frame_info.swapchain_image_view_proxy_id,vk::AttachmentLoadOp::eClear}})
			.set_render_area_extent(viewport_extent_)
			.set_record_func([this](lz::RenderGraph::RenderPassContext context)
			{
				auto shader_program = shader_program_.get();
				auto pipeline_info = this->core_->get_pipeline_cache()->bind_graphics_pipeline(
					context.get_command_buffer(),
					context.get_render_pass()->get_handle(),
					lz::DepthSettings::disabled(),
					{lz::BlendSettings::opaque()},
					lz::VertexDeclaration(),
					vk::PrimitiveTopology::eTriangleList,
					shader_program
				);


				context.get_command_buffer().draw(3, 1, 0, 0);
			}));
	}
	void SimpleRenderer::reload_shaders() 
	{
		/*vertex_shader_.reset(new lz::Shader(core_->get_logical_device(), SHADER_SPIRV_GLSL_DIR"Simple/Simple.vert.spv"));
		fragment_shader_.reset(new lz::Shader(core_->get_logical_device(), SHADER_SPIRV_GLSL_DIR"Simple/Simple.frag.spv"));*/
		vertex_shader_.reset(new lz::Shader(core_->get_logical_device(), SHADER_SPIRV_HLSL_DIR "Simple/Simple.vert.hlsl.spv"));
		fragment_shader_.reset(new lz::Shader(core_->get_logical_device(), SHADER_SPIRV_HLSL_DIR "Simple/Simple.frag.hlsl.spv"));
		shader_program_.reset(new lz::ShaderProgram({ vertex_shader_.get(), fragment_shader_.get() }));
	 }
	void SimpleRenderer::change_view() 
	{
		// we don't need to change view for this renderer
	}
}