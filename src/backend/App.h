#pragma once

#include "backend/LingzeVK.h"

#define GLM_FORCE_RADIANS
#define GLM_DEPTH_ZERO_TO_ONE
//#define GLM_FORCE_RIGHT_HANDED
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/transform.hpp>

#include <iostream>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "json/json.h"
#include <chrono>
#include <ctime>
#include <memory>
#include <sstream>
#include <array>

#include "backend/Core.h"

#include "imgui.h"
#include "backend/ImGuiProfilerRenderer.h"
#include "render/common/ImguiRenderer.h"
#include "scene/Scene.h"
#include "render/renderers/BaseRenderer.h"

namespace lz
{
    // Base application class, contains common functionality for all engine applications
    class App
    {
    public:
        // Constructor
        App(const std::string& appName = "Lingze App", int width = 1280, int height = 760);
        
        // Destructor
        virtual ~App();
        
        // Run the application
        int run();
        
    protected:
        // Initialize the application
        virtual bool init();
        
        // Load scene
        virtual bool load_scene();
        
        // Create renderer
        virtual std::unique_ptr<render::BaseRenderer> create_renderer();
        
        // Update logic
        virtual void update(float deltaTime);
        
        // Render a frame
        virtual void render_frame();
        
        // Process input
        virtual void process_input();
        
        // Clean up resources
        virtual void cleanup();
        
        // GLFW callback static functions
        static void framebuffer_resize_callback(GLFWwindow* window, int width, int height);

        // Helper function to load scene from file
        bool load_scene_from_file(const std::string& config_file_name, lz::Scene::GeometryTypes geo_type);
        
        // Member variables
        std::string app_name_;
        int window_width_;
        int window_height_;
        
        GLFWwindow* window_ = nullptr;
        static bool framebuffer_resized_;
        
        std::unique_ptr<Core> core_;
        std::unique_ptr<render::BaseRenderer> renderer_;
        std::unique_ptr<Scene> scene_;
        std::unique_ptr<render::ImGuiRenderer> imgui_renderer_;
        std::unique_ptr<InFlightQueue> in_flight_queue_;
        
        Camera camera_;
        Camera light_;
    };
} 