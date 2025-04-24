#include "backend/App.h"
#include "backend/ImGuiProfilerRenderer.h"
#include "render/renderers/SimpleRenderer.h"


#include <iostream>
#include <chrono>
#include <sstream>
#include "imgui.h"

namespace lz
{
    // 初始化静态成员变量
    bool App::framebuffer_resized_ = false;

    // 窗口大小变化回调实现
    void App::framebuffer_resize_callback(GLFWwindow* window, int width, int height)
    {
        framebuffer_resized_ = true;
    }

    // 构造函数
    App::App(const std::string& app_name, int width, int height)
        : app_name_(app_name)
        , window_width_(width)
        , window_height_(height)
    {
        // 初始化相机和灯光默认位置
        camera_.pos = glm::vec3(0.0f, 0.5f, -2.0f);
        light_.pos = glm::vec3(0.0f, 0.5f, -2.0f);
    }

    // 析构函数
    App::~App()
    {
        cleanup();
    }

    // 运行应用程序
    int App::run()
    {
        try
        {
            if (!init())
            {
                std::cerr << "应用程序初始化失败！" << std::endl;
                return -1;
            }

            // 主循环
            while (!glfwWindowShouldClose(window_))
            {
                glfwPollEvents();
                process_input();
                render_frame();
            }

            // 等待设备空闲后退出
            if (core_)
            {
                core_->wait_idle();
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "错误: " << e.what() << std::endl;
            return -1;
        }

        return 0;
    }

    // 初始化应用程序
    bool App::init()
    {
        // 初始化GLFW
        if (!glfwInit())
        {
            std::cerr << "GLFW初始化失败" << std::endl;
            return false;
        }

        // 设置GLFW窗口
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window_ = glfwCreateWindow(window_width_, window_height_, app_name_.c_str(), nullptr, nullptr);
        if (!window_)
        {
            std::cerr << "GLFW窗口创建失败" << std::endl;
            glfwTerminate();
            return false;
        }

        // 设置窗口大小变化的回调函数
        glfwSetFramebufferSizeCallback(window_, framebuffer_resize_callback);

        // 设置Vulkan实例和窗口
        const char* glfw_extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };
        uint32_t glfw_extension_count = sizeof(glfw_extensions) / sizeof(glfw_extensions[0]);

        WindowDesc window_desc = {};
        window_desc.h_instance = GetModuleHandle(NULL);
        window_desc.h_wnd = glfwGetWin32Window(window_);

        // 创建Vulkan核心
        bool enable_debugging = true;
        core_ = std::make_unique<Core>(glfw_extensions, glfw_extension_count, &window_desc, enable_debugging);

        // 加载场景
        if (!load_scene())
        {
            std::cerr << "场景加载失败" << std::endl;
            return false;
        }

        // 创建渲染器
        renderer_ = create_renderer();
        if (!renderer_)
        {
            std::cerr << "渲染器创建失败" << std::endl;
            return false;
        }

        // 创建场景资源
        renderer_->recreate_scene_resources(scene_.get());

        // 初始化ImGui渲染器
        imgui_renderer_ = std::make_unique<render::ImGuiRenderer>(core_.get(), window_);

        return true;
    }

    // 加载场景
    bool App::load_scene()
    {
        // 默认实现使用SimpleScene
        // 派生类应重写此方法以加载特定场景
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
            std::cerr << "无法打开场景文件！" << std::endl;
            return false;
        }
        
        bool result = reader.parse(file_stream, config_root);
        if (!result)
        {
            std::cerr << "错误：文件 " << config_file_name << " 解析失败: " << reader.getFormattedErrorMessages() << std::endl;
            return false;
        }

        std::cerr << "文件 " << config_file_name << " 解析成功" << std::endl;
        scene_ = std::make_unique<lz::Scene>(config_root["scene"], core_.get(), geo_type);
        
        return true;
    }

    // 创建渲染器
    std::unique_ptr<render::BaseRenderer> App::create_renderer()
    {
        // 默认创建SimpleRenderer，派生类可以重写此方法
        return std::make_unique<render::SimpleRenderer>(core_.get());
    }

    // 更新逻辑
    void App::update(float deltaTime)
    {
        // 基类中留空，由派生类实现具体逻辑
    }

    // 处理输入
    void App::process_input()
    {
        if (imgui_renderer_)
        {
            imgui_renderer_->process_input(window_);
        }
    }

    // 渲染一帧
    void App::render_frame()
    {
        // 检查是否需要重建交换链 (窗口大小改变或初始化)
        if (!in_flight_queue_ || framebuffer_resized_)
        {
            if (framebuffer_resized_)
            {
                std::cout << "窗口大小改变，重新创建交换链" << std::endl;
                framebuffer_resized_ = false;
                
                if (in_flight_queue_)
                {
                    // 如果已存在交换链，只需重建交换链
                    in_flight_queue_->recreate_swapchain();
                    renderer_->recreate_swapchain_resources(in_flight_queue_->get_image_size(), in_flight_queue_->get_in_flight_frames_count());
                    imgui_renderer_->recreate_swapchain_resources(in_flight_queue_->get_image_size(), in_flight_queue_->get_in_flight_frames_count());
                }
                else
                {
                    // 如果还没有创建交换链，则创建整个队列
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
                std::cout << "创建帧队列" << std::endl;
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
            // 交换链过时，需要重新创建
            core_->wait_idle();
            in_flight_queue_.reset();
            framebuffer_resized_ = true; // 确保重新创建交换链
        }
    }

    // 清理资源
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