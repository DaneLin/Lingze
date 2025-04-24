#include "backend/App.h"
#include "backend/ImGuiProfilerRenderer.h"
#include "render/renderers/SimpleRenderer.h"


#include <iostream>
#include <chrono>
#include <sstream>
#include "imgui.h"

namespace lz
{
    // Initialize static member variables
    bool App::framebuffer_resized_ = false;

    // Window resize callback implementation
    void App::framebuffer_resize_callback(GLFWwindow* window, int width, int height)
    {
        framebuffer_resized_ = true;
    }

    // Constructor
    App::App(const std::string& app_name, int width, int height)
        : app_name_(app_name)
        , window_width_(width)
        , window_height_(height)
    {
        // Initialize default camera and light positions
        camera_.pos = glm::vec3(0.0f, 0.5f, -2.0f);
        light_.pos = glm::vec3(0.0f, 0.5f, -2.0f);
    }

    // Destructor
    App::~App()
    {
        cleanup();
    }

    // Run the application
    int App::run()
    {
        try
        {
            if (!init())
            {
                std::cerr << "Application initialization failed!" << std::endl;
                return -1;
            }

            // Main loop
            while (!glfwWindowShouldClose(window_))
            {
                glfwPollEvents();
                process_input();
                render_frame();
            }

            // Wait for the device to be idle before exiting
            if (core_)
            {
                core_->wait_idle();
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
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
            std::cerr << "GLFW initialization failed" << std::endl;
            return false;
        }

        // Setup GLFW window
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window_ = glfwCreateWindow(window_width_, window_height_, app_name_.c_str(), nullptr, nullptr);
        if (!window_)
        {
            std::cerr << "GLFW window creation failed" << std::endl;
            glfwTerminate();
            return false;
        }

        // Set window resize callback function
        glfwSetFramebufferSizeCallback(window_, framebuffer_resize_callback);

        // Set up Vulkan instance and window
        const char* glfw_extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };
        uint32_t glfw_extension_count = sizeof(glfw_extensions) / sizeof(glfw_extensions[0]);

        WindowDesc window_desc = {};
        window_desc.h_instance = GetModuleHandle(NULL);
        window_desc.h_wnd = glfwGetWin32Window(window_);

        // Create Vulkan core
        bool enable_debugging = true;
        core_ = std::make_unique<Core>(glfw_extensions, glfw_extension_count, &window_desc, enable_debugging);

        // Load scene
        if (!load_scene())
        {
            std::cerr << "Scene loading failed" << std::endl;
            return false;
        }

        // Create renderer
        renderer_ = create_renderer();
        if (!renderer_)
        {
            std::cerr << "Renderer creation failed" << std::endl;
            return false;
        }

        // Create scene resources
        renderer_->recreate_scene_resources(scene_.get());

        // Initialize ImGui renderer
        imgui_renderer_ = std::make_unique<render::ImGuiRenderer>(core_.get(), window_);

        return true;
    }

    // Load scene
    bool App::load_scene()
    {
        // Default implementation uses SimpleScene
        // Derived classes should override this method to load specific scenes
        std::string config_file_name = std::string(SCENE_DIR) + "CubeScene.json";
        Scene::GeometryTypes geo_type = Scene::GeometryTypes::eTriangles;

        Json::Value config_root;
        Json::Reader reader;

        return load_scene_from_file(config_file_name, geo_type);
    }

    bool App::load_scene_from_file(const std::string& config_file_name, lz::Scene::GeometryTypes geo_type)
    {
        Json::Value config_root;
        Json::Reader reader;

        std::ifstream file_stream(config_file_name);
        if (!file_stream.is_open())
        {
            std::cerr << "Unable to open scene file!" << std::endl;
            return false;
        }
        
        bool result = reader.parse(file_stream, config_root);
        if (!result)
        {
            std::cerr << "Error: Failed to parse file " << config_file_name << ": " << reader.getFormattedErrorMessages() << std::endl;
            return false;
        }

        std::cerr << "File " << config_file_name << " parsed successfully" << std::endl;
        scene_ = std::make_unique<lz::Scene>(config_root["scene"], core_.get(), geo_type);
        
        return true;
    }

    // Create renderer
    std::unique_ptr<render::BaseRenderer> App::create_renderer()
    {
        // Default creates SimpleRenderer, derived classes can override this method
        return std::make_unique<render::SimpleRenderer>(core_.get());
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
        }
    }

    // Render a frame
    void App::render_frame()
    {
        // Check if swapchain needs to be rebuilt (window resized or initializing)
        if (!in_flight_queue_ || framebuffer_resized_)
        {
            if (framebuffer_resized_)
            {
                std::cout << "Window resized, recreate swapchain" << std::endl;
                framebuffer_resized_ = false;
                
                if (in_flight_queue_)
                {
                    // If swapchain already exists, just recreate swapchain
                    in_flight_queue_->recreate_swapchain();
                    renderer_->recreate_swapchain_resources(in_flight_queue_->get_image_size(), in_flight_queue_->get_in_flight_frames_count());
                    imgui_renderer_->recreate_swapchain_resources(in_flight_queue_->get_image_size(), in_flight_queue_->get_in_flight_frames_count());
                }
                else
                {
                    // If swapchain hasn't been created yet, create the entire queue
                    core_->clear_caches();
                    WindowDesc window_desc = {};
                    window_desc.h_instance = GetModuleHandle(NULL);
                    window_desc.h_wnd = glfwGetWin32Window(window_);
                    in_flight_queue_ = std::make_unique<InFlightQueue>(core_.get(), window_desc, 2, vk::PresentModeKHR::eMailbox);
                    renderer_->recreate_swapchain_resources(in_flight_queue_->get_image_size(), in_flight_queue_->get_in_flight_frames_count());
                    imgui_renderer_->recreate_swapchain_resources(in_flight_queue_->get_image_size(), in_flight_queue_->get_in_flight_frames_count());
                }
            }
            else
            {
                std::cout << "Create frame queue" << std::endl;
                core_->clear_caches();
                WindowDesc window_desc = {};
                window_desc.h_instance = GetModuleHandle(NULL);
                window_desc.h_wnd = glfwGetWin32Window(window_);
                in_flight_queue_ = std::make_unique<InFlightQueue>(core_.get(), window_desc, 2, vk::PresentModeKHR::eMailbox);
                renderer_->recreate_swapchain_resources(in_flight_queue_->get_image_size(), in_flight_queue_->get_in_flight_frames_count());
                imgui_renderer_->recreate_swapchain_resources(in_flight_queue_->get_image_size(), in_flight_queue_->get_in_flight_frames_count());
            }
        }

        auto& imguiIO = ImGui::GetIO();
        imguiIO.DeltaTime = 1.0f / 60.0f;
        imguiIO.DisplaySize.x = float(in_flight_queue_->get_image_size().width);
        imguiIO.DisplaySize.y = float(in_flight_queue_->get_image_size().height);

        try
        {
            auto frame_info = in_flight_queue_->begin_frame();
            {
                render::ImGuiScopedFrame scoped_frame;

                auto& gpu_profiler_data = in_flight_queue_->get_last_frame_gpu_profiler_data();
                auto& cpu_profiler_data = in_flight_queue_->get_last_frame_cpu_profiler_data();

                {
                    auto pass_creation_task = in_flight_queue_->get_cpu_profiler().start_scoped_task("Pass creation", lz::Colors::orange);
                    renderer_->render_frame(frame_info, camera_, light_, scene_.get(), window_);
                }

                ImGui::Begin("Demo controls", 0, ImGuiWindowFlags_NoScrollbar);
                {
                    ImGui::Text("esdf, c, space: move camera");
                    ImGui::Text("v: live reload shaders");
                }
                ImGui::End();

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
            framebuffer_resized_ = true; // Ensure swapchain recreated
        }
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
} 