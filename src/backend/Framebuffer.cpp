#include "Framebuffer.h"

#include "ImageView.h"

namespace lz
{
	vk::Framebuffer Framebuffer::GetHandle()
	{
		return framebuffer.get();
	}

	Framebuffer::Framebuffer(vk::Device logicalDevice, const std::vector<const ImageView*>& imageViews,
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
}
