#pragma once
#include "GLFW/glfw3.h"
#include "imgui.h"

#include "backend/Core.h"
#include "backend/Config.h"
#include "backend/PresentQueue.h"

namespace lz::render
{
class Sampler;
class Core;

// Frame context class for automatic ImGui frame handling
struct ImGuiScopedFrame
{
	ImGuiScopedFrame()
	{
		ImGui::NewFrame();
	}
	~ImGuiScopedFrame()
	{
		ImGui::EndFrame();
	}
};

class ImGuiRenderer
{
  public:
	ImGuiRenderer(lz::Core *core, GLFWwindow *window);

	~ImGuiRenderer();

	void setup_style();

	void recreate_swapchain_resources(vk::Extent2D viewport_extent, size_t in_flight_frames_count);

	void render_frame(const lz::InFlightQueue::FrameInfo &frame_info, GLFWwindow *window, ImDrawData *draw_data);

	void process_input(GLFWwindow *window);

	void reload_shaders();

  private:
	void init_keymap();

	void init_callbacks(GLFWwindow *window);

	static void key_callback(GLFWwindow *window, int key, int, int action, int mods);

	static void char_callback(GLFWwindow *window, unsigned int c);

	static void mouse_button_callback(GLFWwindow *window, int button, int action, int /*mods*/);

	static void scroll_callback(GLFWwindow *window, double x_offset, double y_offset);

	/*static const char* GetClipboardText()
	{
	  return glfwGetClipboardString(g_Window);
	}

	static void SetClipboardText(const char* text)
	{
	  glfwSetClipboardString(g_Window, text);
	}*/

	void upload_buffers(lz::Buffer *vertex_buffer, lz::Buffer *index_buffer, ImDrawData *draw_data);

	struct InputState
	{
		float  mouse_wheel              = 0.0f;
		double last_update_time         = 0.0;
		bool   mouse_buttons_pressed[3] = {0};
	} input_state_;
#pragma pack(push, 1)
	struct ImGuiVertex
	{
		glm::vec2     pos;
		glm::vec2     uv;
		glm::uint32_t color;
	};
#pragma pack(pop)
	struct FrameResources
	{
		FrameResources(lz::Core *core, size_t max_vertices_count, size_t max_indices_count);

		std::unique_ptr<lz::Buffer> imgui_index_buffer;
		std::unique_ptr<lz::Buffer> imgui_vertex_buffer;
	};
	std::vector<std::unique_ptr<FrameResources>> frame_resources_;

	void load_imgui_font();

	static constexpr uint32_t shader_data_set_index    = 0;
	static constexpr uint32_t draw_call_data_set_index = 1;

	static lz::VertexDeclaration get_imgui_vertex_declaration();

	struct ImGuiShader
	{
#pragma pack(push, 1)
		struct ImGuiShaderData
		{
			glm::mat4 proj_matrix;
		};
#pragma pack(pop)

		std::unique_ptr<lz::Shader>        vertex;
		std::unique_ptr<lz::Shader>        fragment;
		std::unique_ptr<lz::ShaderProgram> program;
	} imgui_shader_;

	vk::Extent2D viewport_extent_;

	std::unique_ptr<lz::ImageView> font_image_view_;
	std::unique_ptr<lz::Image>     font_image_;

	std::unique_ptr<lz::Sampler> image_space_sampler_;
	ImGuiContext                *imgui_context_;
	lz::Core                    *core_;
};
}        // namespace lz::render
