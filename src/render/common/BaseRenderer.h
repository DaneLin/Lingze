#pragma once
#include "GLFW/glfw3.h"
#include "backend/LingzeVK.h"
#include "backend/PresentQueue.h"
#include "scene/Scene.h"
#include "backend/Camera.h"

namespace lz::render
{
/**
 * BaseRenderer - Abstract base class for renderers in the engine
 * Defines the interface that all renderer implementations must follow
 */
class BaseRenderer
{
  public:
	BaseRenderer() = default;

	virtual ~BaseRenderer()
	{}
	
	// Recreates renderer resources when the scene changes
	virtual void recreate_scene_resources(lz::JsonScene *scene)
	{}
	
	// Recreates renderer resources when the swapchain is recreated (e.g. window resize)
	virtual void recreate_swapchain_resources(vk::Extent2D viewport_extent, size_t in_flight_frames_count)
	{}
	
	// Main render function called each frame
	virtual void render_frame(const lz::InFlightQueue::FrameInfo &frame_info, const lz::Camera &camera, const lz::Camera &light, lz::JsonScene *scene, GLFWwindow *window)
	{}
	
	// Reloads shader resources (typically called when shaders are modified)
	virtual void reload_shaders()
	{}
	
	// Changes rendering view/mode
	virtual void change_view()
	{}
};
}        // namespace lz::render
