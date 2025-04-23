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
#include <tiny_obj_loader.h>

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

glm::vec3 ReadJsonVec3f(Json::Value vectorValue)
{
	return glm::vec3(vectorValue[0].asFloat(), vectorValue[1].asFloat(), vectorValue[2].asFloat());
}
glm::ivec2 ReadJsonVec2i(Json::Value vectorValue)
{
	return glm::ivec2(vectorValue[0].asInt(), vectorValue[1].asInt());
}

glm::uvec3 ReadJsonVec3u(Json::Value vectorValue)
{
	return glm::uvec3(vectorValue[0].asUInt(), vectorValue[1].asUInt(), vectorValue[2].asUInt());
}

#include "imgui.h"
#include "backend/ImGuiProfilerRenderer.h"
#include "render/common/ImguiRenderer.h"
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
static bool g_framebufferResized = false;

// 窗口大小变化的回调函数
static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    g_framebufferResized = true;
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
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

    try {
        // 设置Vulkan实例和窗口
        const char* glfwExtensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };
        uint32_t glfwExtensionCount = sizeof(glfwExtensions) / sizeof(glfwExtensions[0]);

        lz::WindowDesc windowDesc = {};
        windowDesc.hInstance = GetModuleHandle(NULL);
        windowDesc.hWnd = glfwGetWin32Window(window);

        // 创建Vulkan核心
        bool enableDebugging = true;
        auto core = std::make_unique<lz::Core>(glfwExtensions, glfwExtensionCount, &windowDesc, enableDebugging);

        lz::ImGuiRenderer imguiRenderer(core.get(), window);
        ImGuiUtils::ProfilersWindow profilersWindow;

        
        std::unique_ptr<lz::InFlightQueue> inFlightQueue;

        glm::f64vec2 mousePos;
        glfwGetCursorPos(window, &mousePos.x, &mousePos.y);

        glm::f64vec2 prevMousePos = mousePos;

        // 简单的主循环
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            
            imguiRenderer.ProcessInput(window);

            // 检查是否需要重建交换链 (窗口大小改变或初始化)
            if (!inFlightQueue || g_framebufferResized)
            {
                if (g_framebufferResized) {
                    std::cout << "Window resized, recreating swapchain" << std::endl;
                    g_framebufferResized = false;
                    
                    if (inFlightQueue) {
                        // 如果已存在交换链，只需重建交换链
                        inFlightQueue->RecreateSwapchain();
                        imguiRenderer.RecreateSwapchainResources(inFlightQueue->GetImageSize(), inFlightQueue->GetInFlightFramesCount());
                    } else {
                        // 如果还没有创建交换链，则创建整个队列
                        core->ClearCaches();
                        inFlightQueue = std::unique_ptr<lz::InFlightQueue>(new lz::InFlightQueue(core.get(), windowDesc, 2, vk::PresentModeKHR::eMailbox));
                        imguiRenderer.RecreateSwapchainResources(inFlightQueue->GetImageSize(), inFlightQueue->GetInFlightFramesCount());
                    }
                } else {
                    std::cout << "Recreating in flight queue" << std::endl;
                    core->ClearCaches();
                    inFlightQueue = std::unique_ptr<lz::InFlightQueue>(new lz::InFlightQueue(core.get(), windowDesc, 2, vk::PresentModeKHR::eMailbox));
                    imguiRenderer.RecreateSwapchainResources(inFlightQueue->GetImageSize(), inFlightQueue->GetInFlightFramesCount());
                }
            }
            

            auto& imguiIO = ImGui::GetIO();
            imguiIO.DeltaTime = 1.0f / 60.0f;              // set the time elapsed since the previous frame (in seconds)
            imguiIO.DisplaySize.x = float(inFlightQueue->GetImageSize().width);             // set the current display width
            imguiIO.DisplaySize.y = float(inFlightQueue->GetImageSize().height);             // set the current display height here

            try
            {
                auto frameInfo = inFlightQueue->BeginFrame();
                {
                    ImGuiScopedFrame scopedFrame;

                    ImGui::Begin("Demo controls", 0, ImGuiWindowFlags_NoScrollbar);
                    {
                        ImGui::Text("esdf, c, space: move camera");
                        ImGui::Text("v: live reload shaders");
                    }
                    ImGui::End();

                    //ImGui::ShowStyleEditor();
                    //ImGui::ShowDemoWindow();


                    ImGui::Render();
                    imguiRenderer.RenderFrame(frameInfo, window, ImGui::GetDrawData());
                }
                inFlightQueue->EndFrame();
            }
            catch (vk::OutOfDateKHRError err)
            {
                // 交换链过时，需要重新创建
                core->WaitIdle();
                inFlightQueue.reset();
                g_framebufferResized = true; // 确保重新创建交换链
            }
            
        }
        
        // 在销毁资源前等待设备空闲
        core->WaitIdle();
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
