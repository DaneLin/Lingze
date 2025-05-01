#pragma once
#include "GLFW/glfw3.h"
#include "backend/LingzeVK.h"
#include "backend/PresentQueue.h"
#include "scene/Scene.h"

namespace lz::render
{
class BaseRenderer
{
  public:
	BaseRenderer() = default;

	virtual ~BaseRenderer()
	{}
	virtual void recreate_scene_resources(lz::Scene *scene)
	{}
	virtual void recreate_swapchain_resources(vk::Extent2D viewport_extent, size_t in_flight_frames_count)
	{}
	virtual void render_frame(const lz::InFlightQueue::FrameInfo &frame_info, const lz::Camera &camera, const lz::Camera &light, lz::Scene *scene, GLFWwindow *window)
	{}
	virtual void reload_shaders()
	{}
	virtual void change_view()
	{}
};
}        // namespace lz::render
