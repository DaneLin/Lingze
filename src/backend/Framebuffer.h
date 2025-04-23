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
		vk::Framebuffer GetHandle();

		// Constructor: Creates a new framebuffer with specified image views
		// Parameters:
		// - logicalDevice: Logical device for creating the framebuffer
		// - imageViews: List of image views to use as attachments
		// - size: Width and height of the framebuffer
		// - renderPass: Render pass this framebuffer is compatible with
		Framebuffer(vk::Device logicalDevice, const std::vector<const ImageView*>& imageViews,
			vk::Extent2D size, vk::RenderPass renderPass);

	private:
		vk::UniqueFramebuffer framebuffer;  // Native Vulkan framebuffer handle
	};

	
}
