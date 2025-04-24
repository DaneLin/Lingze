#include "RenderPassCache.h"

#include "Framebuffer.h"

namespace lz
{
	RenderPassCache::RenderPassKey::RenderPassKey()
	{
	}

	bool RenderPassCache::RenderPassKey::operator<(const RenderPassKey& other) const
	{
		return std::tie(color_attachment_descs, depth_attachment_desc) < std::tie(
			other.color_attachment_descs, other.depth_attachment_desc);
	}

	RenderPassCache::RenderPassCache(vk::Device logical_device): logical_device_(logical_device)
	{
	}

	lz::RenderPass* RenderPassCache::get_render_pass(const RenderPassKey& key)
	{
		auto& render_pass = render_pass_cache_[key];
		if (!render_pass)
		{
			render_pass = std::unique_ptr<lz::RenderPass>(
				new lz::RenderPass(logical_device_, key.color_attachment_descs, key.depth_attachment_desc));
		}
		return render_pass.get();
	}

	FramebufferCache::PassInfo FramebufferCache::begin_pass(vk::CommandBuffer command_buffer,
	                                                       const std::vector<Attachment>& color_attachments, Attachment* depth_attachment, lz::RenderPass* render_pass,
	                                                       vk::Extent2D render_area_extent)
	{
		PassInfo pass_info;

		FramebufferKey framebuffer_key;

		std::vector<vk::ClearValue> clear_values;

		size_t attachmentsUsed = 0;
		for (auto attachment : color_attachments)
		{
			clear_values.push_back(attachment.clear_value);
			framebuffer_key.color_attachment_views[attachmentsUsed++] = attachment.image_view;
		}

		if (depth_attachment)
		{
			framebuffer_key.depth_attachment_view = depth_attachment->image_view;
			clear_values.push_back(depth_attachment->clear_value);
		}

		pass_info.render_pass = render_pass;

		framebuffer_key.extent = render_area_extent;
		framebuffer_key.render_pass = render_pass->get_handle();
		lz::Framebuffer* framebuffer = get_framebuffer(framebuffer_key);
		pass_info.framebuffer = framebuffer;

		const vk::Rect2D rect = vk::Rect2D(vk::Offset2D(), render_area_extent);
		const auto pass_begin_info = vk::RenderPassBeginInfo()
			.setRenderPass(render_pass->get_handle())
			.setFramebuffer(framebuffer->get_handle())
			.setRenderArea(rect)
			.setClearValueCount(static_cast<uint32_t>(clear_values.size()))
			.setPClearValues(clear_values.data());

		command_buffer.beginRenderPass(pass_begin_info, vk::SubpassContents::eInline);

		auto viewport = vk::Viewport()
			.setWidth(float(render_area_extent.width))
			.setHeight(float(render_area_extent.height))
			.setMinDepth(0.0f)
			.setMaxDepth(1.0f);

		command_buffer.setViewport(0, { viewport });
		command_buffer.setScissor(0, { vk::Rect2D(vk::Offset2D(), render_area_extent) });

		return pass_info;
	}

	 void FramebufferCache::end_pass(vk::CommandBuffer command_buffer)
	{
		command_buffer.endRenderPass();
	}

	 FramebufferCache::FramebufferCache(vk::Device logical_device) : logical_device_(logical_device)
	{
	}

	 FramebufferCache::FramebufferKey::FramebufferKey()
	{
		std::fill(color_attachment_views.begin(), color_attachment_views.end(), nullptr);
		depth_attachment_view = nullptr;
		render_pass = nullptr;
	}

	 bool FramebufferCache::FramebufferKey::operator<(const FramebufferKey& other) const
	{
		return std::tie(color_attachment_views, depth_attachment_view, extent.width, extent.height) < std::tie(
			other.color_attachment_views, other.depth_attachment_view, other.extent.width, other.extent.height);
	}

	 lz::Framebuffer* FramebufferCache::get_framebuffer(FramebufferKey key)
	{
		auto& framebuffer = framebuffer_cache_[key];

		if (!framebuffer)
		{
			std::vector<const lz::ImageView*> imageViews;
			for (auto image_view : key.color_attachment_views)
			{
				if (image_view)
				{
					imageViews.push_back(image_view);
				}
			}
			if (key.depth_attachment_view)
			{
				imageViews.push_back(key.depth_attachment_view);
			}

			framebuffer = std::make_unique<lz::Framebuffer>(logical_device_, imageViews, key.extent, key.render_pass);
		}
		return framebuffer.get();
	}
}
