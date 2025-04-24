#include "Framebuffer.h"

#include "ImageView.h"

namespace lz
{
	vk::Framebuffer Framebuffer::get_handle()
	{
		return framebuffer_.get();
	}

	Framebuffer::Framebuffer(const vk::Device logical_device, const std::vector<const ImageView*>& image_views,
	                         const vk::Extent2D size, const vk::RenderPass render_pass)
	{
		// Extract image view handles from our wrapper objects
		std::vector<vk::ImageView> image_view_handles;
		for (const auto image_view : image_views)
		{
			image_view_handles.push_back(image_view->get_handle());
		}

		// Configure framebuffer creation parameters
		const auto framebuffer_info = vk::FramebufferCreateInfo()
		                       .setAttachmentCount(static_cast<uint32_t>(image_view_handles.size()))
		                       .setPAttachments(image_view_handles.data())
		                       .setRenderPass(render_pass)
		                       .setWidth(size.width)
		                       .setHeight(size.height)
		                       .setLayers(1);

		// Create the framebuffer
		this->framebuffer_ = logical_device.createFramebufferUnique(framebuffer_info);
	}
}
