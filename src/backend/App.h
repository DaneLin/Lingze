#pragma once

#include "backend/LingzeVK.h"

#define GLM_FORCE_RADIANS
#define GLM_DEPTH_ZERO_TO_ONE
// #define GLM_FORCE_RIGHT_HANDED
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <iostream>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "json/json.h"
#include <array>
#include <chrono>
#include <ctime>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "backend/Core.h"

#include "backend/ImGuiProfilerRenderer.h"
#include "imgui.h"
#include "render/common/BaseRenderer.h"
#include "render/common/ImguiRenderer.h"
#include "scene/Scene.h"

namespace lz
{
// Base application class, contains common functionality for all engine applications
class App
{
  public:
	// Extension descriptor with optional flag
	struct Extension
	{
		std::string name;
		bool        optional;

		Extension(const std::string &name, bool optional = false) :
		    name(name), optional(optional)
		{}
	};

	// Constructor
	App(const std::string &appName = "Lingze App", int width = 1280, int height = 760);

	// Destructor
	virtual ~App();

	// Run the application
	int run();

	// Add instance extension
	void add_instance_extension(const std::string &name, bool optional = false);

	// Add device extension
	void add_device_extension(const std::string &name, bool optional = false);

	// Clear all instance extensions
	void clear_instance_extensions();

	// Clear all device extensions
	void clear_device_extensions();

  protected:
	// Initialize the application
	virtual bool init();

	// Load scene
	virtual bool load_scene() = 0;

	// Create renderer
	virtual std::unique_ptr<render::BaseRenderer> create_renderer() = 0;

	// Update logic
	virtual void update(float deltaTime);

	// Render a frame
	virtual void render_frame();

	// Render UI
	virtual void render_ui();
	void         recreate_swapchain();

	// Process input
	virtual void process_input();

	// Clean up resources
	virtual void cleanup();

	// GLFW callback static functions
	static void framebuffer_resize_callback(GLFWwindow *window, int width, int height);

	// Helper function to load scene from file
	bool load_scene_from_file(const std::string &config_file_name, lz::Scene::GeometryTypes geo_type);

	// Member variables
	std::string app_name_;
	int         window_width_;
	int         window_height_;

	GLFWwindow *window_ = nullptr;
	static bool framebuffer_resized_;

	std::unique_ptr<Core>                  core_;
	std::unique_ptr<render::BaseRenderer>  renderer_;
	std::unique_ptr<Scene>                 scene_;
	std::unique_ptr<render::ImGuiRenderer> imgui_renderer_;

	std::unique_ptr<InFlightQueue> in_flight_queue_;

	bool                        show_performance = false;
	ImGuiUtils::ProfilersWindow profiler_window_;

	float        delta_time_;
	glm::f64vec2 mouse_pos_;
	glm::f64vec2 prev_mouse_pos_;

	Camera camera_;
	Camera light_;

	// Instance and device extension lists
	std::vector<Extension> instance_extensions_;
	std::vector<Extension> device_extensions_;
};
}        // namespace lz