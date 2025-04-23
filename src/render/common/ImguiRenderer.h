#pragma once
#include "imgui.h"
#include "GLFW/glfw3.h"

#include "backend/LingzeVK.h"
#include "backend/PresentQueue.h"
#include "backend/Core.h"

namespace lz
{
	class Sampler;
	class Core;

	class ImGuiRenderer
    {
    public:
        ImGuiRenderer(lz::Core* _core, GLFWwindow* window);

        ~ImGuiRenderer();

        void SetupStyle();

        void RecreateSwapchainResources(vk::Extent2D viewportExtent, size_t inFlightFramesCount);

        void RenderFrame(const lz::InFlightQueue::FrameInfo& frameInfo, GLFWwindow* window, ImDrawData* drawData);

        void ProcessInput(GLFWwindow* window);

        void ReloadShaders();

    private:
        void InitKeymap();

        void InitCallbacks(GLFWwindow* window);

        static void KeyCallback(GLFWwindow* window, int key, int, int action, int mods);

        static void CharCallback(GLFWwindow* window, unsigned int c);

        static void MouseButtonCallback(GLFWwindow* window, int button, int action, int /*mods*/);

        static void ScrollCallback(GLFWwindow* window, double xOffset, double yOffset);

        /*static const char* GetClipboardText()
        {
          return glfwGetClipboardString(g_Window);
        }

        static void SetClipboardText(const char* text)
        {
          glfwSetClipboardString(g_Window, text);
        }*/

        void UploadBuffers(lz::Buffer* vertexBuffer, lz::Buffer* indexBuffer, ImDrawData* drawData);

        struct InputState
        {
            float mouseWheel = 0.0f;
            double lastUpdateTime = 0.0;
            bool mouseButtonsPressed[3] = { 0 };
        }inputState;
#pragma pack(push, 1)
        struct ImGuiVertex
        {
            glm::vec2 pos;
            glm::vec2 uv;
            glm::uint32_t color;
        };
#pragma pack(pop)
        struct FrameResources
        {
            FrameResources(lz::Core* core, size_t maxVerticesCount, size_t maxIndicesCount);

            std::unique_ptr<lz::Buffer> imGuiIndexBuffer;
            std::unique_ptr<lz::Buffer> imGuiVertexBuffer;
        };
        std::vector<std::unique_ptr<FrameResources> > frameResources;

        void LoadImguiFont();

        const static uint32_t ShaderDataSetIndex = 0;
        const static uint32_t DrawCallDataSetIndex = 1;

        static lz::VertexDeclaration GetImGuiVertexDeclaration();

        struct ImGuiShader
        {
#pragma pack(push, 1)
            struct ImGuiShaderData
            {
                glm::mat4 projMatrix;
            };
#pragma pack(pop)

            std::unique_ptr<lz::Shader> vertex;
            std::unique_ptr<lz::Shader> fragment;
            std::unique_ptr<lz::ShaderProgram> program;
        } imGuiShader;

        vk::Extent2D viewportExtent;

        std::unique_ptr<lz::ImageView> fontImageView;
        std::unique_ptr<lz::Image> fontImage;

        std::unique_ptr<lz::Sampler> imageSpaceSampler;
        ImGuiContext* imguiContext;
        lz::Core* core;
    };
}
