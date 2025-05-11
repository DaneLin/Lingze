#pragma once

#include "backend/ShaderProgram.h"
#include "backend/VertexDeclaration.h"
#include "render/BaseRenderer.h"

namespace lz::render
{
class lz::Core;

class SimpleRenderer : public BaseRenderer
{
  public:
	explicit SimpleRenderer(lz::Core *core);

	virtual void recreate_swapchain_resources(vk::Extent2D viewport_extent, size_t in_flight_frames_count) override;
	virtual void render_frame(const lz::InFlightQueue::FrameInfo &frame_info, const lz::Scene &scene, lz::render::RenderContext &render_context, GLFWwindow *window) override;
	virtual void reload_shaders() override;
	virtual void change_view() override;

  private:
	std::unique_ptr<lz::Shader>        vertex_shader_;
	std::unique_ptr<lz::Shader>        fragment_shader_;
	std::unique_ptr<lz::ShaderProgram> shader_program_;

	vk::Extent2D viewport_extent_;

	lz::Core *core_;
};
}        // namespace lz::render