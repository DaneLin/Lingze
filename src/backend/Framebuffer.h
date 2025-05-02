#pragma once

#include "ImageView.h"
#include "LingzeVK.h"

namespace lz
{
// Framebuffer: Class for managing Vulkan framebuffers
// - Represents a collection of attachments for a render pass
// - Connects specific image views to the attachments defined in a render pass
class Framebuffer
{
  public:
	// GetHandle: Returns the native Vulkan framebuffer handle
	vk::Framebuffer get_handle();

	// Constructor: Creates a new framebuffer with specified image views
	// Parameters:
	// - logicalDevice: Logical device for creating the framebuffer
	// - imageViews: List of image views to use as attachments
	// - size: Width and height of the framebuffer
	// - renderPass: Render pass this framebuffer is compatible with
	Framebuffer(vk::Device logical_device, const std::vector<const ImageView *> &image_views, vk::Extent2D size, vk::RenderPass render_pass);

  private:
	vk::UniqueFramebuffer framebuffer_;        // Native Vulkan framebuffer handle
};
}        // namespace lz
