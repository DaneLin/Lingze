#pragma once
#include <map>

#include "Framebuffer.h"
#include "ImageView.h"
#include "RenderPass.h"

#include "LingzeVK.h"

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

		PassInfo BeginPass(vk::CommandBuffer commandBuffer, const std::vector<Attachment>& colorAttachments,
		                   Attachment* depthAttachment, lz::RenderPass* renderPass, vk::Extent2D renderAreaExtent);

		void EndPass(vk::CommandBuffer commandBuffer);

		FramebufferCache(vk::Device _logicalDevice);

	private:
		struct FramebufferKey
		{
			FramebufferKey();

			bool operator <(const FramebufferKey& other) const;

			std::array<const lz::ImageView*, 8> colorAttachmentViews;
			const lz::ImageView* depthAttachmentView;
			vk::Extent2D extent;
			vk::RenderPass renderPass;
		};

		lz::Framebuffer* GetFramebuffer(FramebufferKey key);

		std::map<FramebufferKey, std::unique_ptr<lz::Framebuffer>> framebufferCache;

		vk::Device logicalDevice;
	};

	
}
