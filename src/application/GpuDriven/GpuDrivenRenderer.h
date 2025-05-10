#pragma once

#include "backend/ShaderProgram.h"
#include "render/BaseRenderer.h"
#include "render/MipBuilder.h"

namespace lz::render
{
class lz::Core;

class GpuDrivenRenderer final : public BaseRenderer
{
  public:
	explicit GpuDrivenRenderer(lz::Core *core);

	void recreate_render_context_resources(lz::render::RenderContext *render_context) override;
	void recreate_swapchain_resources(vk::Extent2D viewport_extent, size_t in_flight_frames_count) override;
	void render_frame(const lz::InFlightQueue::FrameInfo &frame_info, const lz::Camera &camera, const lz::Camera &light, lz::render::RenderContext *render_context, GLFWwindow *window) override;
	void reload_shaders() override;
	void change_view() override;

  private:
	constexpr static uint32_t k_shader_data_set_index = 0;

	struct BasicShapeShader
	{
#pragma pack(push, 1)
		struct DataBuffer
		{
			glm::mat4 view_matrix;
			glm::mat4 proj_matrix;
		};
#pragma pack(pop)
		std::unique_ptr<lz::Shader>        vertex_shader;
		std::unique_ptr<lz::Shader>        fragment_shader;
		std::unique_ptr<lz::ShaderProgram> shader_program;
	} base_shape_shader_;

	struct CullingShader
	{
		std::unique_ptr<lz::Shader> compute_shader;
	} culling_shader_;

	vk::Extent2D viewport_extent_;

	struct SceneResource
	{
		SceneResource(lz::Core *core, lz::render::RenderContext *render_context);

		lz::RenderGraph::BufferProxyUnique visible_mesh_draw_proxy_;
		lz::RenderGraph::BufferProxyUnique mesh_draw_proxy_;
		lz::RenderGraph::BufferProxyUnique mesh_proxy_;
		lz::RenderGraph::BufferProxyUnique visible_mesh_count_proxy_;
	};

	std::unique_ptr<SceneResource> scene_resource_;

	struct ViewportResource
	{
		ViewportResource(lz::RenderGraph *render_graph, glm::uvec2 screen_size) :
		    depth_stencil_proxy_(render_graph, vk::Format::eD32Sfloat,
		                         screen_size, lz::depth_image_usage)
		{}

		UnmippedImageProxy depth_stencil_proxy_;
	};

	std::map<lz::RenderGraph *, std::unique_ptr<ViewportResource>> viewport_resource_datum_;

	lz::Core *core_;
};
}        // namespace lz::render
