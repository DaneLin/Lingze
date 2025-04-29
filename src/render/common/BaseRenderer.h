#pragma once
#include "scene/Scene.h"
#include "backend/PresentQueue.h"
#include "backend/LingzeVK.h"
#include "GLFW/glfw3.h"


namespace lz::render
{
    class BaseRenderer
    {
    public:
        BaseRenderer() = default;

        virtual ~BaseRenderer() {}
        virtual void recreate_scene_resources(lz::Scene* scene) {}
        virtual void recreate_swapchain_resources(vk::Extent2D viewport_extent, size_t in_flight_frames_count) {}
        virtual void render_frame(const lz::InFlightQueue::FrameInfo& frame_info, const lz::Camera& camera, const lz::Camera& light, lz::Scene* scene, GLFWwindow* window) {}
        virtual void reload_shaders() {}
        virtual void change_view() {}
    };
}

