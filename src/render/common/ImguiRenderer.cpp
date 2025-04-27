#include "ImguiRenderer.h"


#include "backend/Sampler.h"
#include "backend/Core.h"
#include "backend/Buffer.h"
#include "backend/VertexDeclaration.h"
#include "backend/PipelineCache.h"
#include "backend/RenderGraph.h"
#include "backend/Pipeline.h"
#include "backend/ShaderMemoryPool.h"
#include "backend/ShaderProgram.h"
#include "backend/DescriptorSetCache.h"
#include "backend/ImageLoader.h"
#include "backend/ImageView.h"


namespace lz::render
{
	ImGuiRenderer::ImGuiRenderer(lz::Core* core, GLFWwindow* window)
	{
		this->core_ = core;

		imgui_context_ = ImGui::CreateContext();
		ImGuiIO& imgui_io = ImGui::GetIO();
		imgui_io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
		imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		imgui_io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
		imgui_io.ConfigFlags |= ImGuiConfigFlags_IsSRGB;
		imgui_io.ConfigWindowsResizeFromEdges = true;
		imgui_io.ConfigDockingTabBarOnSingleWindows = true;
		//imGuiIO.ConfigFlags |= ImGuiConfigFlags_;
		//imGuiIO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		// TODO: Set optional io.ConfigFlags values, e.g. 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard' to enable keyboard controls.
		// TODO: Fill optional fields of the io structure later.
		// TODO: Load TTF/OTF fonts if you don't want to use the default font.

		setup_style();
		ImFontConfig config;
		config.OversampleH = 4;
		config.OversampleV = 4;
		//imGuiIO.Fonts->AddFontFromFileTTF("../data/Fonts/DroidSansMono.ttf", 18.0f, &config);
		//imGuiIO.Fonts->AddFontFromFileTTF("../data/Fonts/Ruda-Bold.ttf", 15.0f, &config);

		load_imgui_font();

		image_space_sampler_.reset(new lz::Sampler(core_->get_logical_device(), vk::SamplerAddressMode::eClampToEdge,
		                                           vk::Filter::eLinear, vk::SamplerMipmapMode::eNearest));
		reload_shaders();

		glfwSetWindowUserPointer(window, &(this->input_state_));
		init_keymap();
		init_callbacks(window);
	}

	ImGuiRenderer::~ImGuiRenderer()
	{
		ImGui::DestroyContext(imgui_context_);
	}

	void ImGuiRenderer::setup_style()
	{
		ImGui::GetStyle().FrameRounding = 4.0f;
		ImGui::GetStyle().GrabRounding = 4.0f;

		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, 0.37f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, 0.16f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, 0.57f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.21f, 0.27f, 0.31f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.55f, 0.73f, 1.00f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.09f, 0.21f, 0.31f, 1.00f);
		colors[ImGuiCol_Tab] = ImVec4(0.07f, 0.10f, 0.15f, 0.00f);
		colors[ImGuiCol_TabActive] = ImVec4(0.19f, 0.41f, 0.78f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.25f, 0.29f, 0.80f);
		colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.29f, 0.55f);
		colors[ImGuiCol_Border] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.12f, 0.14f, 0.65f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.29f, 0.55f);
		colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);


		colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, 0.20f);


		auto* style = &ImGui::GetStyle();

		/*colors[ImGuiCol_Text] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
            colors[ImGuiCol_TextDisabled] = ImVec4(0.500f, 0.500f, 0.500f, 1.000f);
            colors[ImGuiCol_WindowBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
            colors[ImGuiCol_ChildBg] = ImVec4(0.280f, 0.280f, 0.280f, 0.000f);
            colors[ImGuiCol_PopupBg] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
            colors[ImGuiCol_Border] = ImVec4(0.266f, 0.266f, 0.266f, 1.000f);
            colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
            colors[ImGuiCol_FrameBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
            colors[ImGuiCol_FrameBgHovered] = ImVec4(0.200f, 0.200f, 0.200f, 1.000f);
            colors[ImGuiCol_FrameBgActive] = ImVec4(0.280f, 0.280f, 0.280f, 1.000f);
            colors[ImGuiCol_TitleBg] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
            colors[ImGuiCol_TitleBgActive] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
            colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
            colors[ImGuiCol_MenuBarBg] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
            colors[ImGuiCol_ScrollbarBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
            colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.277f, 0.277f, 0.277f, 1.000f);
            colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.300f, 0.300f, 0.300f, 1.000f);
            colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
            colors[ImGuiCol_CheckMark] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
            colors[ImGuiCol_SliderGrab] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
            colors[ImGuiCol_SliderGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
            colors[ImGuiCol_Button] = ImVec4(1.000f, 1.000f, 1.000f, 0.000f);
            colors[ImGuiCol_ButtonHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
            colors[ImGuiCol_ButtonActive] = ImVec4(1.000f, 1.000f, 1.000f, 0.391f);
            colors[ImGuiCol_Header] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
            colors[ImGuiCol_HeaderHovered] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
            colors[ImGuiCol_HeaderActive] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
            colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
            colors[ImGuiCol_SeparatorHovered] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
            colors[ImGuiCol_SeparatorActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
            colors[ImGuiCol_ResizeGrip] = ImVec4(1.000f, 1.000f, 1.000f, 0.250f);
            colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.670f);
            colors[ImGuiCol_ResizeGripActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
            colors[ImGuiCol_Tab] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
            colors[ImGuiCol_TabHovered] = ImVec4(0.352f, 0.352f, 0.352f, 1.000f);
            colors[ImGuiCol_TabActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
            colors[ImGuiCol_TabUnfocused] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
            colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
            //colors[ImGuiCol_DockingPreview] = ImVec4(1.000f, 0.391f, 0.000f, 0.781f);
            //colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
            colors[ImGuiCol_PlotLines] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
            colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
            colors[ImGuiCol_PlotHistogram] = ImVec4(0.586f, 0.586f, 0.586f, 1.000f);
            colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
            colors[ImGuiCol_TextSelectedBg] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
            colors[ImGuiCol_DragDropTarget] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
            colors[ImGuiCol_NavHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
            colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
            colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);
            colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);*/

		/*style->WindowPadding = ImVec2(15, 15);
            style->WindowRounding = 5.0f;
            style->FramePadding = ImVec2(5, 5);
            style->FrameRounding = 4.0f;
            style->ItemSpacing = ImVec2(12, 8);
            style->ItemInnerSpacing = ImVec2(8, 6);
            style->IndentSpacing = 25.0f;
            style->ScrollbarSize = 15.0f;
            style->ScrollbarRounding = 9.0f;
            style->GrabMinSize = 5.0f;
            style->GrabRounding = 3.0f;

            style->Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
            style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
            style->Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
            style->Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
            style->Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
            style->Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
            style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
            style->Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
            style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
            style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
            style->Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
            style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
            style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
            style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
            style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
            style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
            style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
            style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
            //style->Colors[ImGuiCol_ComboBg] = ImVec4(0.19f, 0.18f, 0.21f, 1.00f);
            style->Colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
            style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
            style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
            style->Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
            style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
            style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
            style->Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
            style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
            style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
            //style->Colors[ImGuiCol_Column] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
            //style->Colors[ImGuiCol_ColumnHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
            //style->Colors[ImGuiCol_ColumnActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
            style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
            style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
            //style->Colors[ImGuiCol_CloseButton] = ImVec4(0.40f, 0.39f, 0.38f, 0.16f);
            //style->Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.40f, 0.39f, 0.38f, 0.39f);
            //style->Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.40f, 0.39f, 0.38f, 1.00f);
            style->Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
            style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
            style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
            style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
            style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
            style->Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);*/
	}

	void ImGuiRenderer::recreate_swapchain_resources(vk::Extent2D viewport_extent, size_t in_flight_frames_count)
	{
		this->viewport_extent_ = viewport_extent;
		frame_resources_.clear();

		frame_resources_.resize(in_flight_frames_count);

		for (size_t frameIndex = 0; frameIndex < in_flight_frames_count; frameIndex++)
		{
			frame_resources_[frameIndex].reset(new FrameResources(core_, 150000, 150000));
		}
	}

	void ImGuiRenderer::upload_buffers(lz::Buffer* vertex_buffer, lz::Buffer* index_buffer, ImDrawData* draw_data)
	{
		ImDrawVert* vert_buf_memory = (ImDrawVert*)vertex_buffer->map();
		ImDrawIdx* index_buf_memory = (ImDrawIdx*)index_buffer->map();

		for (int cmd_list_index = 0; cmd_list_index < draw_data->CmdListsCount; cmd_list_index++)
		{
			const ImDrawList* cmd_list = draw_data->CmdLists[cmd_list_index];

			memcpy(vert_buf_memory, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy(index_buf_memory, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));

			vert_buf_memory += cmd_list->VtxBuffer.Size;
			index_buf_memory += cmd_list->IdxBuffer.Size;
		}
		index_buffer->unmap();
		vertex_buffer->unmap();
	}

	lz::VertexDeclaration ImGuiRenderer::get_imgui_vertex_declaration()
	{
		lz::VertexDeclaration vertex_decl;
		vertex_decl.add_vertex_input_binding(0, sizeof(ImDrawVert));
		vertex_decl.add_vertex_attribute(0, offsetof(ImDrawVert, pos), lz::VertexDeclaration::AttribTypes::eVec2, 0);
		vertex_decl.add_vertex_attribute(0, offsetof(ImDrawVert, uv), lz::VertexDeclaration::AttribTypes::eVec2, 1);
		vertex_decl.add_vertex_attribute(0, offsetof(ImDrawVert, col), lz::VertexDeclaration::AttribTypes::eColor32, 2);

		return vertex_decl;
	}

	void ImGuiRenderer::render_frame(const lz::InFlightQueue::FrameInfo& frame_info, GLFWwindow* window,
	                                 ImDrawData* draw_data)
	{
		auto frame_resources_ = this->frame_resources_[frame_info.frame_index].get();
		assert(frame_resources_);

		upload_buffers(frame_resources_->imgui_vertex_buffer.get(), frame_resources_->imgui_index_buffer.get(),
		               draw_data);
		auto vertex_buffer = frame_resources_->imgui_vertex_buffer->get_handle();
		auto index_buffer = frame_resources_->imgui_index_buffer->get_handle();
		core_->get_render_graph()->add_pass(
			lz::RenderGraph::RenderPassDesc()
			.set_color_attachments({frame_info.swapchain_image_view_proxy_id}, vk::AttachmentLoadOp::eLoad)
			.set_render_area_extent(viewport_extent_)
			.set_profiler_info(lz::Colors::peter_river, "ImGuiPass")
			.set_record_func(
				[this, frame_info, draw_data, vertex_buffer, index_buffer](
				lz::RenderGraph::RenderPassContext pass_context)
				{
					const auto pipeline_info = this->core_->get_pipeline_cache()->bind_graphics_pipeline(
						pass_context.get_command_buffer(),
						pass_context.get_render_pass()->get_handle(),
						lz::DepthSettings::disabled(),
						{lz::BlendSettings::alpha_blend()},
						get_imgui_vertex_declaration(),
						vk::PrimitiveTopology::eTriangleList,
						imgui_shader_.program.get());
					{
						const lz::DescriptorSetLayoutKey* shader_data_set_info = imgui_shader_.program->get_set_info(
							shader_data_set_index);
						auto shader_data = frame_info.memory_pool->begin_set(shader_data_set_info);
						{
							auto shader_data_buffer = frame_info.memory_pool->get_uniform_buffer_data<
								ImGuiShader::ImGuiShaderData>("ImGuiShaderData");
							shader_data_buffer->proj_matrix = glm::ortho(0.0f, float(viewport_extent_.width), 0.0f,
							                                           float(viewport_extent_.height));
						}
						frame_info.memory_pool->end_set();

						auto shader_data_set = this->core_->get_descriptor_set_cache()->get_descriptor_set(
							*shader_data_set_info, shader_data.uniform_buffer_bindings, {}, {});

						const lz::DescriptorSetLayoutKey* draw_call_set_info = imgui_shader_.program->get_set_info(
							draw_call_data_set_index);

						uint32_t list_index_offset = 0;
						uint32_t list_vertex_offset = 0;
						for (int cmd_list_index = 0; cmd_list_index < draw_data->CmdListsCount; cmd_list_index++)
						{
							const ImDrawList* cmd_list = draw_data->CmdLists[cmd_list_index];
							const ImDrawVert* vertex_buffer_data = cmd_list->VtxBuffer.Data;
							// vertex buffer generated by Dear ImGui
							const ImDrawIdx* index_buffer_data = cmd_list->IdxBuffer.Data;
							// index buffer generated by Dear ImGui

							for (int cmd_index = 0; cmd_index < cmd_list->CmdBuffer.Size; cmd_index++)
							{
								const ImDrawCmd* draw_cmd = &cmd_list->CmdBuffer[cmd_index];
								if (draw_cmd->UserCallback)
								{
									draw_cmd->UserCallback(cmd_list, draw_cmd);
								}
								else
								{
									// The texture for the draw call is specified by pcmd->TextureId.
									// The vast majority of draw calls will use the Dear ImGui texture atlas, which value you have set yourself during initialization.

									lz::ImageView* tex_image_view = (lz::ImageView*)draw_cmd->TextureId;
									lz::ImageSamplerBinding tex_binding = draw_call_set_info->make_image_sampler_binding(
										"tex", tex_image_view, image_space_sampler_.get());

									auto draw_call_set = this->core_->get_descriptor_set_cache()->get_descriptor_set(
										*draw_call_set_info, {}, {}, {tex_binding});

									pass_context.get_command_buffer().bindDescriptorSets(
										vk::PipelineBindPoint::eGraphics, pipeline_info.pipeline_layout,
										shader_data_set_index,
										{shader_data_set, draw_call_set},
										{shader_data.dynamic_offset});


									// We are using scissoring to clip some objects. All low-level graphics API should supports it.
									// - If your engine doesn't support scissoring yet, you may ignore this at first. You will get some small glitches
									//   (some elements visible outside their bounds) but you can fix that once everything else works!
									// - Clipping coordinates are provided in imGui coordinates space (from draw_data->DisplayPos to draw_data->DisplayPos + draw_data->DisplaySize)
									//   In a single viewport application, draw_data->DisplayPos will always be (0,0) and draw_data->DisplaySize will always be == io.DisplaySize.
									//   However, in the interest of supporting multi-viewport applications in the future (see 'viewport' branch on github),
									//   always subtract draw_data->DisplayPos from clipping bounds to convert them to your viewport space.
									// - Note that pcmd->ClipRect contains Min+Max bounds. Some graphics API may use Min+Max, other may use Min+Size (size being Max-Min)
									//ImVec2 pos = draw_data->DisplayPos;
									//MyEngineScissor((int)(pcmd->ClipRect.x - pos.x), (int)(pcmd->ClipRect.y - pos.y), (int)(pcmd->ClipRect.z - pos.x), (int)(pcmd->ClipRect.w - pos.y));

									// Render 'pcmd->ElemCount/3' indexed triangles.
									// By default the indices ImDrawIdx are 16-bits, you can change them to 32-bits in imconfig.h if your engine doesn't support 16-bits indices.
									//MyEngineDrawIndexedTriangles(pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer, vtx_buffer);

									pass_context.get_command_buffer().bindVertexBuffers(0, {vertex_buffer}, {0});
									pass_context.get_command_buffer().bindIndexBuffer(
										index_buffer, 0,
										sizeof(ImDrawIdx) == 2 ? vk::IndexType::eUint16 : vk::IndexType::eUint32);
									pass_context.get_command_buffer().drawIndexed(
										draw_cmd->ElemCount, 1, list_index_offset + draw_cmd->IdxOffset,
										list_vertex_offset + draw_cmd->VtxOffset, 0);
								}
							}
							list_index_offset += cmd_list->IdxBuffer.Size;
							list_vertex_offset += cmd_list->VtxBuffer.Size;
						}
					}
				}));
	}

	void ImGuiRenderer::process_input(GLFWwindow* window)
	{
		bool is_window_focused = glfwGetWindowAttrib(window, GLFW_FOCUSED);

		auto& imgui_io = ImGui::GetIO();

		glm::f64vec2 mouse_pos;
		glfwGetCursorPos(window, &mouse_pos.x, &mouse_pos.y);

		imgui_io.MousePos = ImVec2(float(mouse_pos.x), float(mouse_pos.y)); // set the mouse position

		//imgui_io.MouseWheel = inputState.mouseWheel;
		//inputState.mouseWheel = 0.0f;

		// Setup time step
		const double curr_time = glfwGetTime();
		imgui_io.DeltaTime = input_state_.last_update_time > 0.0
			                    ? (float)(curr_time - input_state_.last_update_time)
			                    : (float)(1.0f / 60.0f);
		input_state_.last_update_time = curr_time;

		imgui_io.KeyCtrl = imgui_io.KeysDown[GLFW_KEY_LEFT_CONTROL] || imgui_io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
		imgui_io.KeyShift = imgui_io.KeysDown[GLFW_KEY_LEFT_SHIFT] || imgui_io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
		imgui_io.KeyAlt = imgui_io.KeysDown[GLFW_KEY_LEFT_ALT] || imgui_io.KeysDown[GLFW_KEY_RIGHT_ALT];
		imgui_io.KeySuper = imgui_io.KeysDown[GLFW_KEY_LEFT_SUPER] || imgui_io.KeysDown[GLFW_KEY_RIGHT_SUPER];
		/*for (int i = 0; i < 3; i++)
            {
              imgui_io.MouseDown[i] = inputState.mouseButtonsPressed[i] || glfwGetMouseButton(window, i) != 0;    // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
              inputState.mouseButtonsPressed[i] = false;
            }*/

		// Hide OS mouse cursor if ImGui is drawing it
		glfwSetInputMode(window, GLFW_CURSOR, imgui_io.MouseDrawCursor ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);
	}

	void ImGuiRenderer::reload_shaders()
	{
		imgui_shader_.vertex.reset(new lz::Shader(core_->get_logical_device(),SHADER_SPIRV_GLSL_DIR "ImGui/ImGui.vert.spv"));
		imgui_shader_.fragment.reset(new lz::Shader(core_->get_logical_device(),SHADER_SPIRV_GLSL_DIR "ImGui/ImGui.frag.spv"));
		imgui_shader_.program.reset(new lz::ShaderProgram({ imgui_shader_.vertex.get(), imgui_shader_.fragment.get() }));
	}

	void ImGuiRenderer::init_keymap()
	{
		ImGuiIO& imgui_io = ImGui::GetIO();
		imgui_io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
		imgui_io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
		imgui_io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
		imgui_io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
		imgui_io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
		imgui_io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
		imgui_io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
		imgui_io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
		imgui_io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
		imgui_io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
		imgui_io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
		imgui_io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
		imgui_io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
		imgui_io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
		imgui_io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
		imgui_io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
		imgui_io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
		imgui_io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
		imgui_io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
		imgui_io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;
	}

	void ImGuiRenderer::init_callbacks(GLFWwindow* window)
	{
		ImGuiIO& imgui_io = ImGui::GetIO();
		/*imgui_io.SetClipboardTextFn = [this]() {};
            imgui_io.GetClipboardTextFn = GetClipboardText;*/
		/*#ifdef _WIN32
                imgui_io.ImeWindowHandle = glfwGetWin32Window(window);
            #endif*/
		glfwSetMouseButtonCallback(window, mouse_button_callback);
		glfwSetScrollCallback(window, scroll_callback);
		glfwSetKeyCallback(window, key_callback);
		glfwSetCharCallback(window, char_callback);
	}

	void ImGuiRenderer::key_callback(GLFWwindow* window, int key, int, int action, int mods)
	{
		InputState* input_state = (InputState*)glfwGetWindowUserPointer(window);

		ImGuiIO& imgui_io = ImGui::GetIO();
		imgui_io.KeysDown[key] = (action != GLFW_RELEASE);

		(void)mods; // Modifiers are not reliable across systems
	}

	void ImGuiRenderer::char_callback(GLFWwindow* window, unsigned int c)
	{
		InputState* input_state = (InputState*)glfwGetWindowUserPointer(window);

		ImGuiIO& imgui_io = ImGui::GetIO();
		if (c > 0 && c < 0x10000)
			imgui_io.AddInputCharacter((unsigned short)c);
	}

	void ImGuiRenderer::mouse_button_callback(GLFWwindow* window, int button, int action, int)
	{
		InputState* input_state = (InputState*)glfwGetWindowUserPointer(window);

		ImGuiIO& imgui_io = ImGui::GetIO();
		if (button < 512)
		{
			imgui_io.MouseDown[button] = (action != GLFW_RELEASE);
		}
	}

	void ImGuiRenderer::scroll_callback(GLFWwindow* window, double x_offset, double y_offset)
	{
		InputState* input_state = (InputState*)glfwGetWindowUserPointer(window);

		ImGuiIO& imgui_io = ImGui::GetIO();
		imgui_io.MouseWheel += float(y_offset);
		imgui_io.MouseWheelH += float(x_offset);
	}

	ImGuiRenderer::FrameResources::FrameResources(lz::Core* core_, size_t max_vertices_count, size_t max_indices_count)
	{
		imgui_index_buffer = std::make_unique<lz::Buffer>(core_->get_physical_device(),
		                                                  core_->get_logical_device(),
		                                                  sizeof(glm::uint32_t) * max_indices_count,
		                                                  vk::BufferUsageFlagBits::eIndexBuffer,
		                                                  vk::MemoryPropertyFlagBits::eHostVisible |
		                                                  vk::MemoryPropertyFlagBits::eHostCoherent);
		imgui_vertex_buffer = std::make_unique<lz::Buffer>(
			core_->get_physical_device(), core_->get_logical_device(), sizeof(ImGuiVertex) * max_vertices_count,
			vk::BufferUsageFlagBits::eVertexBuffer,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	}

	void ImGuiRenderer::load_imgui_font()
	{
		ImGuiIO& imgui_io = ImGui::GetIO();

		int width, height;
		unsigned char* pixels = nullptr;
		imgui_io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

		auto texel_data = lz::create_simple_image_texel_data(pixels, width, height);
		auto font_create_desc = lz::Image::create_info_2d(texel_data.base_size, uint32_t(texel_data.mips.size()), 1,
		                                                texel_data.format,
		                                                vk::ImageUsageFlagBits::eSampled |
		                                                vk::ImageUsageFlagBits::eTransferDst);

		this->font_image_ = std::make_unique<lz::Image>(core_->get_physical_device(), core_->get_logical_device(),
		                                                font_create_desc);
		lz::load_texel_data(core_, &texel_data, font_image_->get_image_data());
		this->font_image_view_ = std::make_unique<lz::ImageView>(
			core_->get_logical_device(), font_image_->get_image_data(), 0,
			font_image_->get_image_data()->get_mips_count(),
			0, 1);

		imgui_io.Fonts->TexID = (void*)font_image_view_.get();
	}
}
