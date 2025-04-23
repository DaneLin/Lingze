#pragma once

namespace lz
{
	// Framebuffer: Class for managing Vulkan framebuffers
	// - Represents a collection of attachments for a render pass
	// - Connects specific image views to the attachments defined in a render pass
	class Framebuffer
	{
	public:
		// GetHandle: Returns the native Vulkan framebuffer handle
		vk::Framebuffer GetHandle()
		{
			return framebuffer.get();
		}

		// Constructor: Creates a new framebuffer with specified image views
		// Parameters:
		// - logicalDevice: Logical device for creating the framebuffer
		// - imageViews: List of image views to use as attachments
		// - size: Width and height of the framebuffer
		// - renderPass: Render pass this framebuffer is compatible with
		Framebuffer(vk::Device logicalDevice, const std::vector<const ImageView*>& imageViews,
			vk::Extent2D size, vk::RenderPass renderPass)
		{
			// Extract image view handles from our wrapper objects
			std::vector<vk::ImageView> imageViewHandles;
			for (auto imageView : imageViews)
			{
				imageViewHandles.push_back(imageView->GetHandle());
			}

			// Configure framebuffer creation parameters
			auto framebufferInfo = vk::FramebufferCreateInfo()
				.setAttachmentCount(static_cast<uint32_t>(imageViewHandles.size()))
				.setPAttachments(imageViewHandles.data())
				.setRenderPass(renderPass)
				.setWidth(size.width)
				.setHeight(size.height)
				.setLayers(1);

			// Create the framebuffer
			this->framebuffer = logicalDevice.createFramebufferUnique(framebufferInfo);
		}

	private:
		vk::UniqueFramebuffer framebuffer;  // Native Vulkan framebuffer handle
	};
}