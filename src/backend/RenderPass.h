#pragma once

#include "LingzeVK.h"

namespace vk
{
	// Comparison operator for vk::ClearValue to enable sorting and storage in ordered containers
	static bool operator <(const vk::ClearValue& v0, const vk::ClearValue& v1)
	{
		return
			std::tie(v0.color.int32[0], v0.color.int32[1], v0.color.int32[2], v0.color.int32[3]) <
			std::tie(v1.color.int32[0], v1.color.int32[1], v1.color.int32[2], v1.color.int32[3]);
	}
}

namespace lz
{
	// RenderPass: Class for managing Vulkan render passes
	// - Represents a collection of attachments, subpasses, and dependencies
	// - Describes how render targets are used during rendering
	class RenderPass
	{
	public:
		// GetHandle: Returns the native Vulkan render pass handle
		vk::RenderPass GetHandle();

		// GetColorAttachmentsCount: Returns the number of color attachments
		size_t GetColorAttachmentsCount();

		// AttachmentDesc: Structure describing a render pass attachment
		struct AttachmentDesc
		{
			vk::Format format;         // Format of the attachment
			vk::AttachmentLoadOp loadOp; // How the contents of the attachment are initialized
			vk::ClearValue clearValue;   // Clear value used when loadOp is Clear

			// Comparison operator for container ordering
			bool operator <(const AttachmentDesc& other) const;
		};

		// Constructor: Creates a new render pass with specified attachments
		// Parameters:
		// - logicalDevice: Logical device for creating the render pass
		// - colorAttachments: Description of color attachments
		// - depthAttachment: Description of depth attachment (use undefined format for no depth attachment)
		RenderPass(vk::Device logicalDevice, std::vector<AttachmentDesc> colorAttachments,
		           AttachmentDesc depthAttachment);

	private:
		vk::UniqueRenderPass renderPass;  // Native Vulkan render pass handle
		std::vector<AttachmentDesc> colorAttachmentDescs;  // Description of color attachments
		AttachmentDesc depthAttachmentDesc;  // Description of depth attachment
	};
}
