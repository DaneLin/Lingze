#pragma once

#include "render/common/BaseRenderer.h"
#include "backend/VertexDeclaration.h"
#include "backend/Core.h"
#include "backend/ShaderProgram.h"

#include "render/common/MipBuilder.h"

namespace lz::render
{
	class lz::Core;
	class lz::Scene;

	class BasicShapeRenderer : public BaseRenderer
	{
	public:
		explicit BasicShapeRenderer(lz::Core* core);

		virtual void recreate_scene_resources(lz::Scene* scene) override;
		virtual void recreate_swapchain_resources(vk::Extent2D viewport_extent, size_t in_flight_frames_count) override;
		virtual void render_frame(const lz::InFlightQueue::FrameInfo& frame_info, const lz::Camera& camera,
		                          const lz::Camera& light, lz::Scene* scene, GLFWwindow* window) override;
		virtual void reload_shaders() override;
		virtual void change_view() override;

	private:
		constexpr static uint32_t k_shader_data_set_index = 0;
		constexpr static uint32_t k_drawcall_data_set_index = 1;

		lz::VertexDeclaration vertex_decl_;

#pragma pack(push, 1)
		struct DrawCallDataBuffer
		{
			glm::mat4 model_matrix;
		};
#pragma pack(pop)


		struct BasicShapeShader
		{
#pragma pack(push, 1)
			struct DataBuffer
			{
				glm::mat4 view_matrix;
				glm::mat4 proj_matrix;
			};
#pragma pack(pop)

			std::unique_ptr<lz::Shader> vertex_shader;
			std::unique_ptr<lz::Shader> fragment_shader;
			std::unique_ptr<lz::ShaderProgram> shader_program;
		} base_shape_shader_;

		struct FrameResource
		{
			FrameResource(lz::RenderGraph* render_graph, glm::uvec2 screen_size)
				: depth_stencil_proxy_(render_graph, vk::Format::eD32Sfloat, screen_size, lz::depth_image_usage)
			{
			}

			UnmippedImageProxy depth_stencil_proxy_;
		};

		std::map<lz::RenderGraph*, std::unique_ptr<FrameResource>> frame_resource_datum_;

		vk::Extent2D viewport_extent_;

		std::unique_ptr<lz::PipelineCache> pipeline_cache_;

		lz::Core* core_;
	};
}
