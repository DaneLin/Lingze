#pragma once

#include "backend/Sampler.h"
#include "backend/ShaderProgram.h"
#include "render/BaseRenderer.h"
#include "render/MipBuilder.h"

#define NEW_MESH_PATH 1

namespace lz::render
{
class lz::Core;

class MeshShadingRenderer final : public BaseRenderer
{
  public:
	explicit MeshShadingRenderer(lz::Core *core);

	virtual void recreate_swapchain_resources(vk::Extent2D viewport_extent, size_t in_flight_frames_count) override;
	virtual void recreate_render_context_resources(lz::render::RenderContext *render_context) override;
	virtual void render_frame(const lz::InFlightQueue::FrameInfo &frame_info, const lz::Scene &scene, lz::render::RenderContext &render_context, GLFWwindow *window) override;
	virtual void reload_shaders() override;
	virtual void change_view() override;

  private:
	void generate_depth_pyramid(const lz::InFlightQueue::FrameInfo &frame_info, const lz::Scene &scene, lz::render::RenderContext &render_context, lz::RenderGraph *render_graph,
	                            UnmippedImageProxy &depth_stencil_proxy, MippedImageProxy &depth_pyramid_proxy);
	void draw_last_frame_visible(const lz::InFlightQueue::FrameInfo &frame_info, const lz::Scene &scene, lz::render::RenderContext &render_context, lz::RenderGraph *render_graph);
	void draw_last_frame_not_visible(const lz::InFlightQueue::FrameInfo &frame_info, const lz::Scene &scene, lz::render::RenderContext &render_context, lz::RenderGraph *render_graph, MippedImageProxy &depth_pyramid_proxy);
	void draw_mesh_task(const lz::InFlightQueue::FrameInfo &frame_info, const lz::Scene &scene, lz::render::RenderContext &render_context, lz::RenderGraph *render_graph,
	                    UnmippedImageProxy &depth_stencil_proxy, bool late);

	constexpr static uint32_t k_shader_data_set_index    = 0;
	constexpr static uint32_t k_draw_call_data_set_index = 1;

	struct alignas(4) CullData
	{
		glm::mat4 view_matrix;
		float     P00, P11, znear, zfar;        // symmetirc projection parameters
		float     frustum[4];                   // data for left / right / top / bottom
		uint32_t  draw_count;                   // number of draw commands
		float     depth_pyramid_width;
		float     depth_pyramid_height;
	};

	struct DrawCullShader
	{
		std::unique_ptr<lz::Shader> compute_shader;
	} draw_cull_shader_;

	struct DrawCullLateShader
	{
		std::unique_ptr<lz::Shader> compute_shader;
	} draw_cull_late_shader_;

	struct DepthPyramidShader
	{
		std::unique_ptr<lz::Shader> compute_shader;
	} depth_pyramid_shader_;

	struct MeshletShader
	{
		std::unique_ptr<lz::Shader>        task_shader;
		std::unique_ptr<lz::Shader>        mesh_shader;
		std::unique_ptr<lz::Shader>        fragment_shader;
		std::unique_ptr<lz::ShaderProgram> shader_program;
	} meshlet_shader_;

	vk::Extent2D viewport_extent_;

	struct FrameResource
	{
		FrameResource(lz::RenderGraph *render_graph, glm::uvec2 screen_size) :
		    depth_stencil_proxy(render_graph, vk::Format::eD32Sfloat, screen_size, lz::depth_image_usage),
		    depth_pyramid_proxy(render_graph, vk::Format::eR32Sfloat, screen_size, lz::storage_image_usage)
		{}

		UnmippedImageProxy depth_stencil_proxy;
		MippedImageProxy   depth_pyramid_proxy;
	};

	struct SceneResource
	{
		SceneResource(lz::Core *core, lz::render::RenderContext *render_context);

		lz::RenderGraph::BufferProxyUnique visible_meshtask_draw_proxy_;
		lz::RenderGraph::BufferProxyUnique mesh_draw_proxy_;
		lz::RenderGraph::BufferProxyUnique mesh_proxy_;
		lz::RenderGraph::BufferProxyUnique visible_meshtask_count_proxy_;
		lz::RenderGraph::BufferProxyUnique draw_visibility_buffer_proxy_;
	};

	std::unique_ptr<lz::Sampler> depth_reduce_sampler_;

	std::unique_ptr<SceneResource> scene_resource_;

	std::map<lz::RenderGraph *, std::unique_ptr<FrameResource>> frame_resource_datum_;

	lz::Core *core_;
};
}        // namespace lz::render
