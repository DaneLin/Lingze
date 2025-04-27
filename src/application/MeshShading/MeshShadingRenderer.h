#pragma once

#include "render/common/BaseRenderer.h"
#include "backend/ShaderProgram.h"
#include "render/common/MipBuilder.h"

namespace lz::render
{
	class lz::Core;

	class MeshShadingRenderer final : public BaseRenderer
	{
	public:
		explicit MeshShadingRenderer(lz::Core* core);

		virtual void recreate_scene_resources(lz::Scene* scene) override;
		virtual void recreate_swapchain_resources(vk::Extent2D viewport_extent, size_t in_flight_frames_count) override;
		virtual void render_frame(const lz::InFlightQueue::FrameInfo& frame_info, const lz::Camera& camera, const lz::Camera& light, lz::Scene* scene, GLFWwindow* window) override;
		virtual void reload_shaders() override;
		virtual void change_view() override;

		void set_mesh_shading_enable_flag(const bool enable)
		{
			mesh_shading_enable_ = enable;
		}

	private:

		constexpr static uint32_t k_shader_data_set_index = 0;

#pragma pack(push, 1)
		struct DataBuffer
		{
			glm::mat4 view_matrix;
			glm::mat4 proj_matrix;
		};
#pragma pack(pop)

		struct BasicShapeShader
		{
			std::unique_ptr<lz::Shader> vertex_shader;
			std::unique_ptr<lz::Shader> fragment_shader;
			std::unique_ptr<lz::ShaderProgram> shader_program;
		}base_shape_shader_;

		struct MeshletShader
		{
			std::unique_ptr<lz::Shader> mesh_shader;
			std::unique_ptr<lz::Shader> fragment_shader;
			std::unique_ptr<lz::ShaderProgram> shader_program;
		}meshlet_shader_;
		

		vk::Extent2D viewport_extent_;

		struct FrameResource
		{
			FrameResource(lz::RenderGraph* render_graph, glm::uvec2 screen_size)
				: depth_stencil_proxy_(render_graph, vk::Format::eD32Sfloat, screen_size, lz::depth_image_usage)
			{
			}

			UnmippedImageProxy depth_stencil_proxy_;
		};

		std::map<lz::RenderGraph*, std::unique_ptr<FrameResource>> frame_resource_datum_;

		bool mesh_shading_enable_;

		lz::Core* core_;
	};
}
