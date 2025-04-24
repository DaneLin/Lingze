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
    // 应用程序基类，包含所有引擎应用的共同功能
    class App
    {
    public:
        // 构造函数
        App(const std::string& appName = "Lingze App", int width = 1280, int height = 760);
        
        // 析构函数
        virtual ~App();
        
        // 运行应用程序
        int run();
        
    protected:
        // 初始化应用程序
        virtual bool init();
        
        // 加载场景
        virtual bool load_scene();
        
        // 创建渲染器
        virtual std::unique_ptr<render::BaseRenderer> create_renderer();
        
        // 更新逻辑
        virtual void update(float deltaTime);
        
        // 渲染一帧
        virtual void render_frame();
        
        // 处理输入
        virtual void process_input();
        
        // 清理资源
        virtual void cleanup();
        
        // GLFW回调静态函数
        static void framebuffer_resize_callback(GLFWwindow* window, int width, int height);

        // 从文件加载场景的辅助函数
        bool load_scene_from_file(const std::string& config_file_name, lz::Scene::GeometryTypes geo_type);
        
        // 成员变量
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