#include "RenderPass.h"

namespace lz
{
	vk::RenderPass RenderPass::GetHandle()
	{
		return renderPass.get();
	}

	size_t RenderPass::GetColorAttachmentsCount()
	{
		return colorAttachmentDescs.size();
	}

	bool RenderPass::AttachmentDesc::operator<(const AttachmentDesc& other) const
	{
		return
			std::tie(format, loadOp, clearValue) <
			std::tie(other.format, other.loadOp, other.clearValue);
	}

	RenderPass::RenderPass(vk::Device logicalDevice, std::vector<AttachmentDesc> colorAttachments,
		AttachmentDesc depthAttachment)
	{
		this->colorAttachmentDescs = colorAttachments;
		this->depthAttachmentDesc = depthAttachment;

		std::vector<vk::AttachmentReference> colorAttachmentRefs;

		uint32_t currAttachmentIndex = 0;

		std::vector<vk::AttachmentDescription> attachmentDescs;
		for (auto colorAttachmentDesc : colorAttachmentDescs)
		{
			// Create reference to color attachment
			colorAttachmentRefs.push_back(vk::AttachmentReference()
			                              .setAttachment(currAttachmentIndex++)
			                              .setLayout(vk::ImageLayout::eColorAttachmentOptimal));

			// Create description for color attachment
			auto attachmentDesc = vk::AttachmentDescription()
			                      .setFormat(colorAttachmentDesc.format)
			                      .setSamples(vk::SampleCountFlagBits::e1)
			                      .setLoadOp(colorAttachmentDesc.loadOp)
			                      .setStoreOp(vk::AttachmentStoreOp::eStore)
			                      .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
			                      .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
			                      .setInitialLayout(vk::ImageLayout::eColorAttachmentOptimal)
			                      .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);
			attachmentDescs.push_back(attachmentDesc);

			// Configure subpass with color attachments
			auto subpass = vk::SubpassDescription()
			               .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
			               .setColorAttachmentCount(uint32_t(colorAttachmentRefs.size()))
			               .setPColorAttachments(colorAttachmentRefs.data());

			vk::AttachmentDescription depthDesc;
			vk::AttachmentReference depthRef;
				
			// Add depth attachment if format is specified
			if (depthAttachmentDesc.format != vk::Format::eUndefined)
			{
				/*auto srcAccessPattern = GetImageAccessPattern(depthAttachmentDesc.srcUsageType, true);
					auto dstAccessPattern = GetImageAccessPattern(depthAttachmentDesc.dstUsageType, false);*/

				// Create reference to depth attachment
				depthRef
					.setAttachment(currAttachmentIndex++)
					.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

				// Create description for depth attachment
				auto attachmentDesc = vk::AttachmentDescription()
				                      .setFormat(depthAttachmentDesc.format)
				                      .setSamples(vk::SampleCountFlagBits::e1)
				                      .setLoadOp(depthAttachmentDesc.loadOp)
				                      .setStoreOp(vk::AttachmentStoreOp::eStore)
				                      .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
				                      .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
				                      /*.setInitialLayout(srcAccessPattern.layout)
					                      .setFinalLayout(dstAccessPattern.layout)*/
				                      .setInitialLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
				                      .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
				attachmentDescs.push_back(attachmentDesc);

				// Set depth attachment in subpass
				subpass
					.setPDepthStencilAttachment(&depthRef);

				/*subpassDependency.srcStageMask |= srcAccessPattern.stage;
					subpassDependency.srcAccessMask |= srcAccessPattern.accessMask;
					subpassDependency.dstStageMask |= dstAccessPattern.stage;
					subpassDependency.dstAccessMask |= dstAccessPattern.accessMask;*/
			}

			// Create render pass with the configured attachments and subpass
			auto renderPassInfo = vk::RenderPassCreateInfo()
			                      .setAttachmentCount(uint32_t(attachmentDescs.size()))
			                      .setPAttachments(attachmentDescs.data())
			                      .setSubpassCount(1)
			                      .setPSubpasses(&subpass)
			                      .setDependencyCount(0)
			                      .setPDependencies(nullptr);
			//.setDependencyCount(1)
			//.setPDependencies(&subpassDependency);

			this->renderPass = logicalDevice.createRenderPassUnique(renderPassInfo);
		}
	}
}
