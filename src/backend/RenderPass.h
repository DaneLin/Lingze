#pragma once
namespace vk
{
	static bool operator <(const vk::ClearValue& v0, const vk::ClearValue& v1)
	{
		return
			std::tie(v0.color.int32[0], v0.color.int32[1], v0.color.int32[2], v0.color.int32[3]) <
			std::tie(v1.color.int32[0], v1.color.int32[1], v1.color.int32[2], v1.color.int32[3]);
	}
}

namespace lz
{
	class RenderPass
	{
	public:
		vk::RenderPass GetHandle()
		{
			return renderPass.get();
		}

		size_t GetColorAttachmentsCount()
		{
			return colorAttachmentDescs.size();
		}

		struct AttachmentDesc
		{
			vk::Format format;
			vk::AttachmentLoadOp loadOp;
			vk::ClearValue clearValue;

			bool operator <(const AttachmentDesc& other) const
			{
				return
					std::tie(format, loadOp, clearValue) <
					std::tie(other.format, other.loadOp, other.clearValue);
			}
		};

		RenderPass(vk::Device logicalDevice, std::vector<AttachmentDesc> colorAttachments,
		           AttachmentDesc depthAttachment)
		{
			this->colorAttachmentDescs = colorAttachments;
			this->depthAttachmentDesc = depthAttachment;

			std::vector<vk::AttachmentReference> colorAttachmentRefs;

			uint32_t currAttachmentIndex = 0;

			std::vector<vk::AttachmentDescription> attachmentDescs;
			for (auto colorAttachmentDesc : colorAttachmentDescs)
			{
				colorAttachmentRefs.push_back(vk::AttachmentReference()
				                              .setAttachment(currAttachmentIndex++)
				                              .setLayout(vk::ImageLayout::eColorAttachmentOptimal));

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

				auto subpass = vk::SubpassDescription()
				               .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
				               .setColorAttachmentCount(uint32_t(colorAttachmentRefs.size()))
				               .setPColorAttachments(colorAttachmentRefs.data());

				vk::AttachmentDescription depthDesc;
				vk::AttachmentReference depthRef;
				if (depthAttachmentDesc.format != vk::Format::eUndefined)
				{
					/*auto srcAccessPattern = GetImageAccessPattern(depthAttachmentDesc.srcUsageType, true);
					auto dstAccessPattern = GetImageAccessPattern(depthAttachmentDesc.dstUsageType, false);*/

					depthRef
						.setAttachment(currAttachmentIndex++)
						.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

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

					subpass
						.setPDepthStencilAttachment(&depthRef);

					/*subpassDependency.srcStageMask |= srcAccessPattern.stage;
					subpassDependency.srcAccessMask |= srcAccessPattern.accessMask;
					subpassDependency.dstStageMask |= dstAccessPattern.stage;
					subpassDependency.dstAccessMask |= dstAccessPattern.accessMask;*/
				}

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

	private:
		vk::UniqueRenderPass renderPass;
		std::vector<AttachmentDesc> colorAttachmentDescs;
		AttachmentDesc depthAttachmentDesc;
	};
}
