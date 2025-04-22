#pragma once
#include <map>
#include "RenderPass.h"

namespace lz
{
	class RenderPassCache
	{
	public:
		struct RenderPassKey
		{
			RenderPassKey()
			{
			}

			std::vector<lz::RenderPass::AttachmentDesc> colorAttachmentDescs;
			lz::RenderPass::AttachmentDesc depthAttachmentDesc;

			bool operator<(const RenderPassKey& other) const
			{
				return std::tie(colorAttachmentDescs, depthAttachmentDesc) < std::tie(
					other.colorAttachmentDescs, other.depthAttachmentDesc);
			}
		};

		RenderPassCache(vk::Device logicalDevice)
			: logicalDevice(logicalDevice)
		{
		}

		lz::RenderPass* GetRenderPass(const RenderPassKey& key)
		{
			auto& renderPass = renderPassCache[key];
			if (!renderPass)
			{
				renderPass = std::unique_ptr<lz::RenderPass>(
					new lz::RenderPass(logicalDevice, key.colorAttachmentDescs, key.depthAttachmentDesc));
			}
			return renderPass.get();
		}

	private:
		std::map<RenderPassKey, std::unique_ptr<lz::RenderPass>> renderPassCache;
		vk::Device logicalDevice;
	};

	class FramebufferCache
	{
	public:
		struct PassInfo
		{
			lz::Framebuffer* framebuffer;
			lz::RenderPass* renderPass;
		};

		struct Attachment
		{
			lz::ImageView* imageView;
			vk::ClearValue clearValue;
		};

		PassInfo BeginPass(vk::CommandBuffer commandBuffer, const std::vector<Attachment> colorAttachments,
		                   Attachment* depthAttachment, lz::RenderPass* renderPass, vk::Extent2D renderAreaExtent)
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

			commandBuffer.setViewport(0, {viewport});
			commandBuffer.setScissor(0, {vk::Rect2D(vk::Offset2D(), renderAreaExtent)});

			return passInfo;
		}

		void EndPass(vk::CommandBuffer commandBuffer)
		{
			commandBuffer.endRenderPass();
		}

		FramebufferCache(vk::Device _logicalDevice) : logicalDevice(_logicalDevice)
		{
		}

	private:
		struct FramebufferKey
		{
			FramebufferKey()
			{
				std::fill(colorAttachmentViews.begin(), colorAttachmentViews.end(), nullptr);
				depthAttachmentView = nullptr;
				renderPass = nullptr;
			}

			bool operator <(const FramebufferKey& other) const
			{
				return std::tie(colorAttachmentViews, depthAttachmentView, extent.width, extent.height) < std::tie(
					other.colorAttachmentViews, other.depthAttachmentView, other.extent.width, other.extent.height);
			}

			std::array<const lz::ImageView*, 8> colorAttachmentViews;
			const lz::ImageView* depthAttachmentView;
			vk::Extent2D extent;
			vk::RenderPass renderPass;
		};

		lz::Framebuffer* GetFramebuffer(FramebufferKey key)
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

				framebuffer = std::unique_ptr<lz::Framebuffer>(
					new lz::Framebuffer(logicalDevice, imageViews, key.extent, key.renderPass));
			}
			return framebuffer.get();
		}

		std::map<FramebufferKey, std::unique_ptr<lz::Framebuffer>> framebufferCache;

		vk::Device logicalDevice;
	};
}
