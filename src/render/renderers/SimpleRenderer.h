#pragma once

#include "BaseRenderer.h"
#include "backend/VertexDeclaration.h"
#include "backend/Core.h"
#include "backend/ShaderProgram.h"

namespace lz::render
{
	class SimpleRenderer : public BaseRenderer
	{
	public:

		SimpleRenderer(lz::Core* core);

		virtual void recreate_scene_resources(lz::Scene* scene) override;
		virtual void recreate_swapchain_resources(vk::Extent2D viewport_extent, size_t in_flight_frames_count) override;
		virtual void render_frame(const lz::InFlightQueue::FrameInfo& frame_info, const lz::Camera& camera, const lz::Camera& light, lz::Scene* scene, GLFWwindow* window) override;
		virtual void reload_shaders() override;
		virtual void change_view() override;

	private:
		constexpr static uint32_t k_shader_data_set_index = 0;

		lz::VertexDeclaration vertex_decl_;

		std::unique_ptr<lz::Shader> vertex_shader;
		std::unique_ptr<lz::Shader> fragment_shader;
		std::unique_ptr<lz::ShaderProgram> program;

		vk::Extent2D viewport_extent_;

		lz::Core* core_;
	};
}