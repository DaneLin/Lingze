// Lingze.cpp: 定义应用程序的入口点。
//
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
#include "render/renderers/SimpleRenderer.h"
using namespace std;

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

// 窗口大小变化的全局标志
static bool g_framebuffer_resized = false;

// 窗口大小变化的回调函数
static void framebuffer_resize_callback(GLFWwindow* window, int width, int height) {
    g_framebuffer_resized = true;
}

std::unique_ptr<lz::render::BaseRenderer> create_renderer(lz::Core* core, std::string name)
{
    if (name == "SimpleRenderer")
    {
        return std::unique_ptr<lz::render::SimpleRenderer>(new lz::render::SimpleRenderer(core));
    }
    assert(!"Wrong renderer specified");
    return nullptr;
}

int main()
{
    // 初始化GLFW
    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed" << std::endl;
        return -1;
    }

    // 设置GLFW窗口
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1280, 760, "Lingze", nullptr, nullptr);
    if (!window) {
        std::cerr << "GLFW window creation failed" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    // 设置窗口大小变化的回调函数
    glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);

    


    try {
        // 设置Vulkan实例和窗口
        const char* glfw_extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };
        uint32_t glfw_extension_count = sizeof(glfw_extensions) / sizeof(glfw_extensions[0]);

        lz::WindowDesc window_desc = {};
        window_desc.h_instance = GetModuleHandle(NULL);
        window_desc.h_wnd = glfwGetWin32Window(window);

        // 创建Vulkan核心
        bool enable_debugging = true;
        auto core = std::make_unique<lz::Core>(glfw_extensions, glfw_extension_count, &window_desc, enable_debugging);

        // Loading Scene
        std::string config_file_name;
        lz::Scene::GeometryTypes geo_type;
        std::string renderer_name;

        config_file_name = std::string(SCENE_DIR) + "CubeScene.json";
        geo_type = lz::Scene::GeometryTypes::eTriangles;
        renderer_name = "SimpleRenderer";

        Json::Value config_root;
        Json::Reader reader;

        std::ifstream file_stream(config_file_name);
        if (!file_stream.is_open())
        {
            std::cerr << "Can't open scene file!\n";
        }
        bool result = reader.parse(file_stream, config_root);
        if (result)
        {
            std::cerr << "File " << config_file_name << ", parsing successfully\n";
        }
        else
        {
            std::cerr << "Error : File " << config_file_name << ", parsing failed with errors: " << reader.getFormattedErrorMessages() << "\n";
        }

        lz::Scene scene(config_root["scene"], core.get(), geo_type);

        std::unique_ptr<lz::render::BaseRenderer> renderer;

        renderer = create_renderer(core.get(), renderer_name);

        renderer->recreate_scene_resources(&scene);

        // Init imgui renderer
        lz::ImGuiRenderer imgui_renderer(core.get(), window);
        ImGuiUtils::ProfilersWindow profilers_window;
        
        std::unique_ptr<lz::InFlightQueue> in_flight_queue;

        glm::f64vec2 mouse_pos;
        glfwGetCursorPos(window, &mouse_pos.x, &mouse_pos.y);

        glm::f64vec2 prev_mouse_pos = mouse_pos;

        lz::Camera light;
        light.pos = glm::vec3(0.0f, 0.5f, -2.0f);

        lz::Camera camera;
        camera.pos = glm::vec3(0.0f, 0.5f, -2.0f);

        // 简单的主循环
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            
            imgui_renderer.process_input(window);

            // 检查是否需要重建交换链 (窗口大小改变或初始化)
            if (!in_flight_queue || g_framebuffer_resized)
            {
                if (g_framebuffer_resized) {
                    std::cout << "Window resized, recreating swapchain" << std::endl;
                    g_framebuffer_resized = false;
                    
                    if (in_flight_queue) {
                        // 如果已存在交换链，只需重建交换链
                        in_flight_queue->recreate_swapchain();
                        renderer->recreate_swapchain_resources(in_flight_queue->get_image_size(), in_flight_queue->get_in_flight_frames_count());
                        imgui_renderer.recreate_swapchain_resources(in_flight_queue->get_image_size(), in_flight_queue->get_in_flight_frames_count());
                    } else {
                        // 如果还没有创建交换链，则创建整个队列
                        core->clear_caches();
                        in_flight_queue = std::unique_ptr<lz::InFlightQueue>(new lz::InFlightQueue(core.get(), window_desc, 2, vk::PresentModeKHR::eMailbox));
                        renderer->recreate_swapchain_resources(in_flight_queue->get_image_size(), in_flight_queue->get_in_flight_frames_count());
                        imgui_renderer.recreate_swapchain_resources(in_flight_queue->get_image_size(), in_flight_queue->get_in_flight_frames_count());
                    }
                } else {
                    std::cout << "Recreating in flight queue" << std::endl;
                    core->clear_caches();
                    in_flight_queue = std::unique_ptr<lz::InFlightQueue>(new lz::InFlightQueue(core.get(), window_desc, 2, vk::PresentModeKHR::eMailbox));
                    renderer->recreate_swapchain_resources(in_flight_queue->get_image_size(), in_flight_queue->get_in_flight_frames_count());
                    imgui_renderer.recreate_swapchain_resources(in_flight_queue->get_image_size(), in_flight_queue->get_in_flight_frames_count());
                }
            }
            

            auto& imguiIO = ImGui::GetIO();
            imguiIO.DeltaTime = 1.0f / 60.0f;              // set the time elapsed since the previous frame (in seconds)
            imguiIO.DisplaySize.x = float(in_flight_queue->get_image_size().width);             // set the current display width
            imguiIO.DisplaySize.y = float(in_flight_queue->get_image_size().height);             // set the current display height here

            try
            {
                auto frame_info = in_flight_queue->begin_frame();
                {
                    ImGuiScopedFrame scoped_frame;

                    auto& gpu_profiler_data = in_flight_queue->get_last_frame_gpu_profiler_data();
                    auto& cpu_profiler_data = in_flight_queue->get_last_frame_cpu_profiler_data();

                    {
                        auto pass_creation_task = in_flight_queue->get_cpu_profiler().start_scoped_task("Pass creation",lz::Colors::orange);
                        renderer->render_frame(frame_info, camera, light, &scene, window);
                    }

                    ImGui::Begin("Demo controls", 0, ImGuiWindowFlags_NoScrollbar);
                    {
                        ImGui::Text("esdf, c, space: move camera");
                        ImGui::Text("v: live reload shaders");
                    }
                    ImGui::End();

                    //ImGui::ShowStyleEditor();
                    //ImGui::ShowDemoWindow();


                    ImGui::Render();
                    imgui_renderer.render_frame(frame_info, window, ImGui::GetDrawData());
                }
                in_flight_queue->end_frame();
            }
            catch (vk::OutOfDateKHRError err)
            {
                // 交换链过时，需要重新创建
                core->wait_idle();
                in_flight_queue.reset();
                g_framebuffer_resized = true; // 确保重新创建交换链
            }
            
        }
        
        // 在销毁资源前等待设备空闲
        core->wait_idle();
    }
    catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << std::endl;
        return -1;
    }

    // 清理资源
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}
