﻿#include "backend/App.h"
#include "backend/ImGuiProfilerRenderer.h"
#include "backend/Logging.h"

#include "App.h"
#include "imgui.h"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <sstream>

namespace lz
{
// Initialize static member variables
bool App::framebuffer_resized_ = false;

// Window resize callback implementation
void App::framebuffer_resize_callback(GLFWwindow *window, int width, int height)
{
	framebuffer_resized_ = true;
}

// Constructor
App::App(const std::string &app_name, int width, int height) :
    app_name_(app_name), window_width_(width), window_height_(height)
{
	// Initialize default camera and light positions
	camera_.pos = glm::vec3(0.0f, 0.5f, -2.0f);
	light_.pos  = glm::vec3(0.0f, 0.5f, -2.0f);

	// Add default instance extensions required by GLFW
	add_instance_extension("VK_KHR_surface");
	add_instance_extension("VK_KHR_win32_surface");

	// Add default device extensions
	add_device_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	spdlog::set_pattern(LOGGER_FORMAT);
}

// Destructor
App::~App()
{
	cleanup();
}

// Add instance extension
void App::add_instance_extension(const std::string &name, bool optional)
{
	instance_extensions_.emplace_back(name, optional);
}

// Add device extension
void App::add_device_extension(const std::string &name, bool optional)
{
	device_extensions_.emplace_back(name, optional);
}

// Clear all instance extensions
void App::clear_instance_extensions()
{
	instance_extensions_.clear();
}

// Clear all device extensions
void App::clear_device_extensions()
{
	device_extensions_.clear();
}

// Run the application
int App::run()
{
	try
	{
		if (!init())
		{
			LOGE("Application initialization failed!");
			return -1;
		}

		auto prev_frame_time = std::chrono::system_clock::now();
		;

		// Main loop
		while (!glfwWindowShouldClose(window_))
		{
			auto curr_frame_time = std::chrono::system_clock::now();
			delta_time_          = std::chrono::duration<float>(curr_frame_time - prev_frame_time).count();
			prev_frame_time      = curr_frame_time;

			glfwPollEvents();
			recreate_swapchain();
			process_input();
			render_frame();
		}

		// Wait for the device to be idle before exiting
		if (core_)
		{
			core_->wait_idle();
		}
	}
	catch (const std::exception &e)
	{
		LOGE("Error: {}", e.what());
		return -1;
	}

	return 0;
}

// Initialize the application
bool App::init()
{
	// Initialize GLFW
	if (!glfwInit())
	{
		LOGE("GLFW initialization failed");
		return false;
	}

	// Setup GLFW window
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window_ = glfwCreateWindow(window_width_, window_height_, app_name_.c_str(), nullptr, nullptr);
	if (!window_)
	{
		LOGE("GLFW window creation failed");
		glfwTerminate();
		return false;
	}

	// Set window resize callback function
	glfwSetFramebufferSizeCallback(window_, framebuffer_resize_callback);

	// Prepare instance extensions
	std::vector<const char *> instance_extension_names;
	for (const auto &ext : instance_extensions_)
	{
		instance_extension_names.push_back(ext.name.c_str());
	}

	// Window setup for surface creation
	WindowDesc window_desc = {};
	window_desc.h_instance = GetModuleHandle(NULL);
	window_desc.h_wnd      = glfwGetWin32Window(window_);

	// Create Vulkan core
	bool enable_debugging = true;

	// Prepare device extensions
	std::vector<const char *> device_extension_names;
	for (const auto &ext : device_extensions_)
	{
		device_extension_names.push_back(ext.name.c_str());
	}

	// Create the Core with our extension lists
	core_ = std::make_unique<Core>(
	    instance_extension_names.data(),
	    static_cast<uint32_t>(instance_extension_names.size()),
	    &window_desc,
	    enable_debugging,
	    device_extension_names);

	// Create render context
	render_context_ = std::make_unique<render::RenderContext>(core_.get());

	// Prepare scene
	prepare_render_context();

	// Create renderer
	renderer_ = create_renderer();
	if (!renderer_)
	{
		LOGE("Renderer creation failed");
		return false;
	}

	// Create scene resources
	renderer_->recreate_render_context_resources(render_context_.get());

	// Initialize ImGui renderer
	imgui_renderer_ = std::make_unique<render::ImGuiRenderer>(core_.get(), window_);

	glfwGetCursorPos(window_, &mouse_pos_.x, &mouse_pos_.y);
	prev_mouse_pos_ = mouse_pos_;

	return true;
}

void App::prepare_render_context()
{
}

// Update logic
void App::update(float deltaTime)
{
	// Empty in base class, implemented by derived classes
}

// Process input
void App::process_input()
{
	if (imgui_renderer_)
	{
		imgui_renderer_->process_input(window_);

		glfwGetCursorPos(window_, &mouse_pos_.x, &mouse_pos_.y);

		auto &imgui_io         = ImGui::GetIO();
		imgui_io.DeltaTime     = 1.0f / 60.0f;
		imgui_io.DisplaySize.x = float(in_flight_queue_->get_image_size().width);
		imgui_io.DisplaySize.y = float(in_flight_queue_->get_image_size().height);

		if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
		{
			glm::vec3 dir = glm::vec3(0.0f, 0.0f, 0.0f);

			if (glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_2))
			{
				float mouse_speed = 0.01f;
				if (mouse_pos_ != prev_mouse_pos_)
				{
					renderer_->change_view();
				}
				camera_.hor_angle += float((mouse_pos_ - prev_mouse_pos_).x * mouse_speed);
				camera_.vert_angle += float((mouse_pos_ - prev_mouse_pos_).y * mouse_speed);
			}
			glm::mat4 camera_transform = camera_.get_transform_matrix();
			glm::vec3 camera_forward   = glm::vec3(camera_transform * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
			glm::vec3 camera_right     = glm::vec3(camera_transform * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
			glm::vec3 camera_up        = glm::vec3(camera_transform * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));

			if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS)
			{
				dir += glm::vec3(0.0f, 0.0f, 1.0f);
			}
			if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS)
			{
				dir += glm::vec3(0.0f, 0.0f, -1.0f);
			}
			if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS)
			{
				dir += glm::vec3(-1.0f, 0.0f, 0.0f);
			}
			if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS)
			{
				dir += glm::vec3(1.0f, 0.0f, 0.0f);
			}
			if (glfwGetKey(window_, GLFW_KEY_Q) == GLFW_PRESS)
			{
				dir += glm::vec3(0.0f, -1.0f, 0.0f);
			}
			if (glfwGetKey(window_, GLFW_KEY_E) == GLFW_PRESS)
			{
				dir += glm::vec3(0.0f, 1.0f, 0.0f);
			}

			if (glm::length(dir) > 0.0f)
			{
				renderer_->change_view();
			}

			float camera_speed = 3.0f;
			camera_.pos += camera_forward * dir.z * camera_speed * delta_time_;
			camera_.pos += camera_right * dir.x * camera_speed * delta_time_;
			camera_.pos += camera_up * dir.y * camera_speed * delta_time_;

			if (glfwGetKey(window_, GLFW_KEY_V) == GLFW_PRESS)
			{
				renderer_->reload_shaders();
			}
		}
	}
}

// Render UI
void App::render_ui()
{
	ImGui::Begin("Demo controls", 0, ImGuiWindowFlags_NoScrollbar);
	{
		ImGui::Text("wasd, q, e: move camera");
		ImGui::Text("v: live reload shaders");

		ImGui::Checkbox("Show performance", &show_performance);

		// TODO: Add more status
	}
	ImGui::End();
}

void App::recreate_swapchain()
{
	// Check if swapchain needs to be rebuilt (window resized or initializing)
	if (!in_flight_queue_ || framebuffer_resized_)
	{
		if (framebuffer_resized_)
		{
			LOGI("Window resized, recreate swapchain");
			framebuffer_resized_ = false;

			if (in_flight_queue_)
			{
				// If swapchain already exists, just recreate swapchain
				in_flight_queue_->recreate_swapchain();
				renderer_->recreate_swapchain_resources(in_flight_queue_->get_image_size(),
				                                        in_flight_queue_->get_in_flight_frames_count());
				imgui_renderer_->recreate_swapchain_resources(in_flight_queue_->get_image_size(),
				                                              in_flight_queue_->get_in_flight_frames_count());
			}
			else
			{
				// If swapchain hasn't been created yet, create the entire queue
				core_->clear_caches();
				WindowDesc window_desc = {};
				window_desc.h_instance = GetModuleHandle(NULL);
				window_desc.h_wnd      = glfwGetWin32Window(window_);
				in_flight_queue_       = std::make_unique<InFlightQueue>(core_.get(), window_desc, 2,
                                                                   vk::PresentModeKHR::eMailbox);
				renderer_->recreate_swapchain_resources(in_flight_queue_->get_image_size(),
				                                        in_flight_queue_->get_in_flight_frames_count());
				imgui_renderer_->recreate_swapchain_resources(in_flight_queue_->get_image_size(),
				                                              in_flight_queue_->get_in_flight_frames_count());
			}
		}
		else
		{
			core_->clear_caches();
			WindowDesc window_desc = {};
			window_desc.h_instance = GetModuleHandle(NULL);
			window_desc.h_wnd      = glfwGetWin32Window(window_);
			in_flight_queue_       = std::make_unique<InFlightQueue>(core_.get(), window_desc, 2,
                                                               vk::PresentModeKHR::eMailbox);
			renderer_->recreate_swapchain_resources(in_flight_queue_->get_image_size(),
			                                        in_flight_queue_->get_in_flight_frames_count());
			imgui_renderer_->recreate_swapchain_resources(in_flight_queue_->get_image_size(),
			                                              in_flight_queue_->get_in_flight_frames_count());
		}
	}
}

// Render a frame
void App::render_frame()
{
	try
	{
		auto frame_info = in_flight_queue_->begin_frame();
		{
			render::ImGuiScopedFrame scoped_frame;
			{
				auto pass_creation_task = in_flight_queue_->get_cpu_profiler().start_scoped_task(
				    "Pass creation", lz::Colors::orange);
				renderer_->render_frame(frame_info, camera_, light_, render_context_.get(), window_);
			}

			// Performance statistics
			if (show_performance)
			{
				auto &gpu_profiler_data = in_flight_queue_->get_last_frame_gpu_profiler_data();
				auto &cpu_profiler_data = in_flight_queue_->get_last_frame_cpu_profiler_data();

				if (!profiler_window_.stop_profiling)
				{
					auto profilers_task = in_flight_queue_->get_cpu_profiler().start_scoped_task(
					    "Performance processing", lz::Colors::sun_flower);

					profiler_window_.cpu_graph.load_frame_data(cpu_profiler_data.data(), cpu_profiler_data.size());
					profiler_window_.gpu_graph.load_frame_data(gpu_profiler_data.data(), gpu_profiler_data.size());
				}

				{
					auto profilers_task = in_flight_queue_->get_cpu_profiler().start_scoped_task(
					    "Performance rendering", lz::Colors::belize_hole);
					profiler_window_.render();
				}
			}

			render_ui();

			ImGui::Render();
			imgui_renderer_->render_frame(frame_info, window_, ImGui::GetDrawData());
		}
		in_flight_queue_->end_frame();
	}
	catch (vk::OutOfDateKHRError err)
	{
		// Swapchain outdated, need to recreate
		core_->wait_idle();
		in_flight_queue_.reset();
		framebuffer_resized_ = true;        // Ensure swapchain recreated
	}

	prev_mouse_pos_ = mouse_pos_;
}

// Clean up resources
void App::cleanup()
{
	if (core_)
	{
		core_->wait_idle();
	}

	if (window_)
	{
		glfwDestroyWindow(window_);
		window_ = nullptr;
	}

	glfwTerminate();
}

}        // namespace lz
