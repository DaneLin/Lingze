#include "RenderPass.h"

namespace lz
{
	vk::RenderPass RenderPass::get_handle()
	{
		return render_pass_.get();
	}

	size_t RenderPass::get_color_attachments_count() const
	{
		return color_attachment_descs_.size();
	}

	bool RenderPass::AttachmentDesc::operator<(const AttachmentDesc& other) const
	{
		return
			std::tie(format, load_op, clear_value) <
			std::tie(other.format, other.load_op, other.clear_value);
	}

	RenderPass::RenderPass(vk::Device logical_device, std::vector<AttachmentDesc> color_attachments,
	                       AttachmentDesc depth_attachment)
	{
		this->color_attachment_descs_ = color_attachments;
		this->depth_attachment_desc_ = depth_attachment;

		std::vector<vk::AttachmentReference> colorAttachmentRefs;

		uint32_t currAttachmentIndex = 0;

		std::vector<vk::AttachmentDescription> attachmentDescs;
		for (auto colorAttachmentDesc : color_attachment_descs_)
		{
			// Create reference to color attachment
			colorAttachmentRefs.push_back(vk::AttachmentReference()
			                              .setAttachment(currAttachmentIndex++)
			                              .setLayout(vk::ImageLayout::eColorAttachmentOptimal));

			// Create description for color attachment
			auto attachmentDesc = vk::AttachmentDescription()
			                      .setFormat(colorAttachmentDesc.format)
			                      .setSamples(vk::SampleCountFlagBits::e1)
			                      .setLoadOp(colorAttachmentDesc.load_op)
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
			if (depth_attachment_desc_.format != vk::Format::eUndefined)
			{
				/*auto srcAccessPattern = GetImageAccessPattern(depth_attachment_desc_.srcUsageType, true);
					auto dstAccessPattern = GetImageAccessPattern(depth_attachment_desc_.dstUsageType, false);*/

				// Create reference to depth attachment
				depthRef
					.setAttachment(currAttachmentIndex++)
					.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

				// Create description for depth attachment
				auto attachmentDesc = vk::AttachmentDescription()
				                      .setFormat(depth_attachment_desc_.format)
				                      .setSamples(vk::SampleCountFlagBits::e1)
				                      .setLoadOp(depth_attachment_desc_.load_op)
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
			auto render_pass_info = vk::RenderPassCreateInfo()
			                        .setAttachmentCount(uint32_t(attachmentDescs.size()))
			                        .setPAttachments(attachmentDescs.data())
			                        .setSubpassCount(1)
			                        .setPSubpasses(&subpass)
			                        .setDependencyCount(0)
			                        .setPDependencies(nullptr);
			//.setDependencyCount(1)
			//.setPDependencies(&subpassDependency);

			this->render_pass_ = logical_device.createRenderPassUnique(render_pass_info);
		}
	}
}
