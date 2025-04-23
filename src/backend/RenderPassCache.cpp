#include "RenderPassCache.h"

#include "Framebuffer.h"

namespace lz
{
	 FramebufferCache::PassInfo FramebufferCache::BeginPass(vk::CommandBuffer commandBuffer,
		const std::vector<Attachment>& colorAttachments, Attachment* depthAttachment, lz::RenderPass* renderPass,
		vk::Extent2D renderAreaExtent)
	{
		PassInfo passInfo;

		FramebufferKey framebufferKey;

		std::vector<vk::ClearValue> clearValues;

		size_t attachmentsUsed = 0;
		for (auto attachment : colorAttachments)
		{
			clearValues.push_back(attachment.clearValue);
			framebufferKey.colorAttachmentViews[attachmentsUsed++] = attachment.imageView;
		}

		if (depthAttachment)
		{
			framebufferKey.depthAttachmentView = depthAttachment->imageView;
			clearValues.push_back(depthAttachment->clearValue);
		}

		passInfo.renderPass = renderPass;

		framebufferKey.extent = renderAreaExtent;
		framebufferKey.renderPass = renderPass->GetHandle();
		lz::Framebuffer* framebuffer = GetFramebuffer(framebufferKey);
		passInfo.framebuffer = framebuffer;

		vk::Rect2D rect = vk::Rect2D(vk::Offset2D(), renderAreaExtent);
		auto passBeginInfo = vk::RenderPassBeginInfo()
			.setRenderPass(renderPass->GetHandle())
			.setFramebuffer(framebuffer->GetHandle())
			.setRenderArea(rect)
			.setClearValueCount(static_cast<uint32_t>(clearValues.size()))
			.setPClearValues(clearValues.data());

		commandBuffer.beginRenderPass(passBeginInfo, vk::SubpassContents::eInline);

		auto viewport = vk::Viewport()
			.setWidth(float(renderAreaExtent.width))
			.setHeight(float(renderAreaExtent.height))
			.setMinDepth(0.0f)
			.setMaxDepth(1.0f);

		commandBuffer.setViewport(0, { viewport });
		commandBuffer.setScissor(0, { vk::Rect2D(vk::Offset2D(), renderAreaExtent) });

		return passInfo;
	}

	 void FramebufferCache::EndPass(vk::CommandBuffer commandBuffer)
	{
		commandBuffer.endRenderPass();
	}

	 FramebufferCache::FramebufferCache(vk::Device _logicalDevice) : logicalDevice(_logicalDevice)
	{
	}

	 FramebufferCache::FramebufferKey::FramebufferKey()
	{
		std::fill(colorAttachmentViews.begin(), colorAttachmentViews.end(), nullptr);
		depthAttachmentView = nullptr;
		renderPass = nullptr;
	}

	 bool FramebufferCache::FramebufferKey::operator<(const FramebufferKey& other) const
	{
		return std::tie(colorAttachmentViews, depthAttachmentView, extent.width, extent.height) < std::tie(
			other.colorAttachmentViews, other.depthAttachmentView, other.extent.width, other.extent.height);
	}

	 lz::Framebuffer* FramebufferCache::GetFramebuffer(FramebufferKey key)
	{
		auto& framebuffer = framebufferCache[key];

		if (!framebuffer)
		{
			std::vector<const lz::ImageView*> imageViews;
			for (auto imageView : key.colorAttachmentViews)
			{
				if (imageView)
				{
					imageViews.push_back(imageView);
				}
			}
			if (key.depthAttachmentView)
			{
				imageViews.push_back(key.depthAttachmentView);
			}

			framebuffer = std::make_unique<lz::Framebuffer>(logicalDevice, imageViews, key.extent, key.renderPass);
		}
		return framebuffer.get();
	}
}
