#pragma once

#include "Config.h"

namespace lz
{
// QueueFamilyTypes: Enumeration of different Vulkan queue family types
// - Used to identify which queue a command should be submitted to
enum struct QueueFamilyTypes : uint8_t
{
	eGraphics,        // Graphics queue family (vertex/fragment shaders, drawing)
	eTransfer,        // Transfer queue family (memory transfers, copies)
	eCompute,         // Compute queue family (compute shaders)
	ePresent,         // Presentation queue family (presenting to display)
	eUndefined        // Undefined queue family
};

// ImageAccessPattern: Structure describing how an image is accessed
// - Contains information about pipeline stages, access flags, layout, and queue family
struct ImageAccessPattern
{
	vk::PipelineStageFlags stage;                    // Pipeline stage where the image is accessed
	vk::AccessFlags        access_mask;              // Memory access flags for the image
	vk::ImageLayout        layout;                   // Layout the image should be in
	QueueFamilyTypes       queue_family_type;        // Queue family that performs the access
};

// ImageSubresourceBarrier: Structure for barrier between two image access patterns
// - Describes transition from one access pattern to another
struct ImageSubresourceBarrier
{
	ImageAccessPattern access_pattern;            // Source access pattern
	ImageAccessPattern dst_access_pattern;        // Destination access pattern
};

// ImageUsageTypes: Enumeration of different ways an image can be used
// - Used to identify how an image is being used at a particular point
enum struct ImageUsageTypes : uint8_t
{
	eGraphicsShaderRead,             // Read from a graphics shader (vertex/fragment)
	eGraphicsShaderReadWrite,        // Read and write from a graphics shader
	eComputeShaderRead,              // Read from a compute shader
	eComputeShaderReadWrite,         // Read and write from a compute shader
	eTransferDst,                    // Destination of a transfer operation
	eTransferSrc,                    // Source of a transfer operation
	eColorAttachment,                // Used as a color attachment
	eDepthAttachment,                // Used as a depth attachment
	ePresent,                        // Used for presentation
	eNone,                           // Not used
	eUnknown                         // Usage is unknown or unspecified
};

// GetSrcImageAccessPattern: Creates an access pattern for the source of an image barrier
// Parameters:
// - usageTypes: How the image is used at the source of the barrier
// Returns: Appropriate access pattern for the specified usage
static ImageAccessPattern get_src_image_access_pattern(const ImageUsageTypes usage_types)
{
	ImageAccessPattern access_pattern;
	switch (usage_types)
	{
		case ImageUsageTypes::eGraphicsShaderRead:
		{
			// Image was read by fragment shader
			access_pattern.stage             = vk::PipelineStageFlagBits::eFragmentShader;
			access_pattern.access_mask       = vk::AccessFlags();
			access_pattern.layout            = vk::ImageLayout::eShaderReadOnlyOptimal;
			access_pattern.queue_family_type = QueueFamilyTypes::eGraphics;
		}
		break;
		case ImageUsageTypes::eGraphicsShaderReadWrite:
		{
			// Image was read/written by graphics shaders
			access_pattern.stage = vk::PipelineStageFlagBits::eVertexShader |
			                       vk::PipelineStageFlagBits::eFragmentShader;
			access_pattern.access_mask       = vk::AccessFlagBits::eShaderWrite;
			access_pattern.layout            = vk::ImageLayout::eGeneral;
			access_pattern.queue_family_type = QueueFamilyTypes::eGraphics;
		}
		break;
		case ImageUsageTypes::eComputeShaderRead:
		{
			// Image was read by compute shader
			access_pattern.stage             = vk::PipelineStageFlagBits::eComputeShader;
			access_pattern.access_mask       = vk::AccessFlags();
			access_pattern.layout            = vk::ImageLayout::eShaderReadOnlyOptimal;
			access_pattern.queue_family_type = QueueFamilyTypes::eCompute;
		}
		break;
		case ImageUsageTypes::eComputeShaderReadWrite:
		{
			// Image was read/written by compute shader
			access_pattern.stage             = vk::PipelineStageFlagBits::eComputeShader;
			access_pattern.access_mask       = vk::AccessFlagBits::eShaderWrite;
			access_pattern.layout            = vk::ImageLayout::eGeneral;
			access_pattern.queue_family_type = QueueFamilyTypes::eCompute;
		}
		break;
		case ImageUsageTypes::eTransferSrc:
		{
			// Image was source of a transfer
			access_pattern.stage             = vk::PipelineStageFlagBits::eTransfer;
			access_pattern.access_mask       = vk::AccessFlags();
			access_pattern.layout            = vk::ImageLayout::eTransferSrcOptimal;
			access_pattern.queue_family_type = QueueFamilyTypes::eTransfer;
		}
		break;
		case ImageUsageTypes::eTransferDst:
		{
			// Image was destination of a transfer
			access_pattern.stage             = vk::PipelineStageFlagBits::eTransfer;
			access_pattern.access_mask       = vk::AccessFlagBits::eTransferWrite;
			access_pattern.layout            = vk::ImageLayout::eTransferDstOptimal;
			access_pattern.queue_family_type = QueueFamilyTypes::eTransfer;
		}
		break;
		case ImageUsageTypes::eColorAttachment:
		{
			// Image was used as a color attachment
			access_pattern.stage             = vk::PipelineStageFlagBits::eColorAttachmentOutput;
			access_pattern.access_mask       = vk::AccessFlagBits::eColorAttachmentWrite;
			access_pattern.layout            = vk::ImageLayout::eColorAttachmentOptimal;
			access_pattern.queue_family_type = QueueFamilyTypes::eGraphics;
		}
		break;
		case ImageUsageTypes::eDepthAttachment:
		{
			// Image was used as a depth attachment
			access_pattern.stage             = vk::PipelineStageFlagBits::eLateFragmentTests;
			access_pattern.access_mask       = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
			access_pattern.layout            = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			access_pattern.queue_family_type = QueueFamilyTypes::eGraphics;
		}
		break;
		case ImageUsageTypes::ePresent:
		{
			// Image was used for presentation
			access_pattern.stage             = vk::PipelineStageFlagBits::eBottomOfPipe;
			access_pattern.access_mask       = vk::AccessFlags();
			access_pattern.layout            = vk::ImageLayout::ePresentSrcKHR;
			access_pattern.queue_family_type = QueueFamilyTypes::ePresent;
		}
		break;
		case ImageUsageTypes::eNone:
		{
			// Image was not used
			access_pattern.stage             = vk::PipelineStageFlagBits::eTopOfPipe;
			access_pattern.access_mask       = vk::AccessFlags();
			access_pattern.layout            = vk::ImageLayout::eUndefined;
			access_pattern.queue_family_type = QueueFamilyTypes::eUndefined;
		}
		break;
		case ImageUsageTypes::eUnknown:
		{
			// Image usage is unknown
			access_pattern.stage             = vk::PipelineStageFlagBits::eBottomOfPipe;
			access_pattern.access_mask       = vk::AccessFlags();
			access_pattern.layout            = vk::ImageLayout::eUndefined;
			access_pattern.queue_family_type = QueueFamilyTypes::eUndefined;
		}
		break;
	};
	return access_pattern;
}

// GetDstImageAccessPattern: Creates an access pattern for the destination of an image barrier
// Parameters:
// - usageType: How the image will be used after the barrier
// Returns: Appropriate access pattern for the specified usage
static ImageAccessPattern get_dst_image_access_pattern(const ImageUsageTypes usage_type)
{
	ImageAccessPattern access_pattern;
	switch (usage_type)
	{
		case ImageUsageTypes::eGraphicsShaderRead:
		{
			// Image will be read by vertex shader
			access_pattern.stage             = vk::PipelineStageFlagBits::eVertexShader;
			access_pattern.access_mask       = vk::AccessFlagBits::eShaderRead;
			access_pattern.layout            = vk::ImageLayout::eShaderReadOnlyOptimal;
			access_pattern.queue_family_type = QueueFamilyTypes::eGraphics;
		}
		break;
		case ImageUsageTypes::eGraphicsShaderReadWrite:
		{
			// Image will be read/written by graphics shaders
			access_pattern.stage = vk::PipelineStageFlagBits::eVertexShader |
			                       vk::PipelineStageFlagBits::eFragmentShader;
			access_pattern.access_mask       = vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;
			access_pattern.layout            = vk::ImageLayout::eGeneral;
			access_pattern.queue_family_type = QueueFamilyTypes::eGraphics;
		}
		break;
		case ImageUsageTypes::eComputeShaderRead:
		{
			// Image will be read by compute shader
			access_pattern.stage             = vk::PipelineStageFlagBits::eComputeShader;
			access_pattern.access_mask       = vk::AccessFlagBits::eShaderRead;
			access_pattern.layout            = vk::ImageLayout::eShaderReadOnlyOptimal;
			access_pattern.queue_family_type = QueueFamilyTypes::eCompute;
		}
		break;
		case ImageUsageTypes::eComputeShaderReadWrite:
		{
			// Image will be read/written by compute shader
			access_pattern.stage             = vk::PipelineStageFlagBits::eComputeShader;
			access_pattern.access_mask       = vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead;
			access_pattern.layout            = vk::ImageLayout::eGeneral;
			access_pattern.queue_family_type = QueueFamilyTypes::eCompute;
		}
		break;
		case ImageUsageTypes::eTransferDst:
		{
			// Image will be destination of a transfer
			access_pattern.stage             = vk::PipelineStageFlagBits::eTransfer;
			access_pattern.access_mask       = vk::AccessFlagBits::eTransferWrite;
			access_pattern.layout            = vk::ImageLayout::eTransferDstOptimal;
			access_pattern.queue_family_type = QueueFamilyTypes::eTransfer;
		}
		break;
		case ImageUsageTypes::eTransferSrc:
		{
			// Image will be source of a transfer
			access_pattern.stage             = vk::PipelineStageFlagBits::eTransfer;
			access_pattern.access_mask       = vk::AccessFlagBits::eTransferRead;
			access_pattern.layout            = vk::ImageLayout::eTransferSrcOptimal;
			access_pattern.queue_family_type = QueueFamilyTypes::eTransfer;
		}
		break;
		case ImageUsageTypes::eColorAttachment:
		{
			// Image will be used as a color attachment
			access_pattern.stage       = vk::PipelineStageFlagBits::eColorAttachmentOutput;
			access_pattern.access_mask = vk::AccessFlagBits::eColorAttachmentRead |
			                             vk::AccessFlagBits::eColorAttachmentWrite;
			access_pattern.layout            = vk::ImageLayout::eColorAttachmentOptimal;
			access_pattern.queue_family_type = QueueFamilyTypes::eGraphics;
		}
		break;
		case ImageUsageTypes::eDepthAttachment:
		{
			// Image will be used as a depth attachment
			access_pattern.stage = vk::PipelineStageFlagBits::eLateFragmentTests |
			                       vk::PipelineStageFlagBits::eEarlyFragmentTests;
			access_pattern.access_mask = vk::AccessFlagBits::eDepthStencilAttachmentRead |
			                             vk::AccessFlagBits::eDepthStencilAttachmentWrite;
			access_pattern.layout            = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			access_pattern.queue_family_type = QueueFamilyTypes::eGraphics;
		}
		break;
		case ImageUsageTypes::ePresent:
		{
			// Image will be used for presentation
			access_pattern.stage             = vk::PipelineStageFlagBits::eBottomOfPipe;
			access_pattern.access_mask       = vk::AccessFlags();
			access_pattern.layout            = vk::ImageLayout::ePresentSrcKHR;
			access_pattern.queue_family_type = QueueFamilyTypes::ePresent;
		}
		break;
		case ImageUsageTypes::eNone:
		{
			// Image will not be used
			access_pattern.stage             = vk::PipelineStageFlagBits::eBottomOfPipe;
			access_pattern.access_mask       = vk::AccessFlags();
			access_pattern.layout            = vk::ImageLayout::eUndefined;
			access_pattern.queue_family_type = QueueFamilyTypes::eUndefined;
		}
		break;
		case ImageUsageTypes::eUnknown:
		{
			// Image usage is unknown
			assert(0);
			access_pattern.stage             = vk::PipelineStageFlagBits::eTopOfPipe;
			access_pattern.access_mask       = vk::AccessFlags();
			access_pattern.layout            = vk::ImageLayout::eUndefined;
			access_pattern.queue_family_type = QueueFamilyTypes::eUndefined;
		}
		break;
	};
	return access_pattern;
}

/**
 * @brief Determine if a barrier is needed between two image usage types
 * @param src_usage_type Source image usage type
 * @param dst_usage_type Destination image usage type
 * @return Returns true if a barrier is needed, otherwise false
 */
static bool is_image_barrier_needed(const ImageUsageTypes src_usage_type, const ImageUsageTypes dst_usage_type)
{
	if (src_usage_type == ImageUsageTypes::eGraphicsShaderRead && dst_usage_type == ImageUsageTypes::eGraphicsShaderRead)
		return false;
	return true;
}

/**
 * @brief Buffer access pattern
 * Describes how a buffer is accessed at a pipeline stage and the associated access masks and queue type
 */
struct BufferAccessPattern
{
	vk::PipelineStageFlags stage;                    // Pipeline stage that accesses the buffer
	vk::AccessFlags        access_mask;              // Memory access flags
	QueueFamilyTypes       queue_family_type;        // Queue family type that performs the access
};

/**
 * @brief Buffer barrier
 * Describes barrier information needed to transition from one access pattern to another
 */
struct BufferBarrier
{
	ImageAccessPattern src_access_pattern;        // Source access pattern
	ImageAccessPattern dst_access_pattern;        // Destination access pattern
};

/**
 * @brief Buffer usage type enumeration
 * Defines different usage types for buffers in Vulkan pipeline
 */
enum struct BufferUsageTypes : uint8_t
{
	eVertexBuffer,                   // Used as vertex buffer
	eGraphicsShaderReadWrite,        // Used by graphics shader for read/write
	eComputeShaderReadWrite,         // Used by compute shader for read/write
	eTransferDst,                    // Used as transfer destination
	eTransferSrc,                    // Used as transfer source
	eIndirectBuffer,                 // Used as indirect buffer (e.g., visible_mesh_count_buffer in MeshShading)
	eNone,                           // Not used
	eUnknown                         // Unknown usage
};

/**
 * @brief Get source access pattern for a buffer
 * @param usage_type Buffer usage type
 * @return Source access pattern
 */
static BufferAccessPattern get_src_buffer_access_pattern(const BufferUsageTypes usage_type)
{
	BufferAccessPattern access_pattern;
	switch (usage_type)
	{
		case BufferUsageTypes::eVertexBuffer:
		{
			// Buffer used as vertex buffer
			access_pattern.stage             = vk::PipelineStageFlagBits::eVertexInput;
			access_pattern.access_mask       = vk::AccessFlags();
			access_pattern.queue_family_type = QueueFamilyTypes::eGraphics;
		}
		break;
		case BufferUsageTypes::eGraphicsShaderReadWrite:
		{
			// Buffer will be read/written by graphics shader
			access_pattern.stage = vk::PipelineStageFlagBits::eVertexShader |
			                       vk::PipelineStageFlagBits::eFragmentShader;
			access_pattern.access_mask       = vk::AccessFlagBits::eShaderWrite;
			access_pattern.queue_family_type = QueueFamilyTypes::eGraphics;
		}
		break;
		case BufferUsageTypes::eComputeShaderReadWrite:
		{
			// Buffer will be read/written by compute shader
			access_pattern.stage             = vk::PipelineStageFlagBits::eComputeShader;
			access_pattern.access_mask       = vk::AccessFlagBits::eShaderWrite;
			access_pattern.queue_family_type = QueueFamilyTypes::eCompute;
		}
		break;
		case BufferUsageTypes::eIndirectBuffer:
		{
			// Buffer used for indirect commands (like visible_mesh_count_buffer in MeshShading)
			access_pattern.stage             = vk::PipelineStageFlagBits::eDrawIndirect;
			access_pattern.access_mask       = vk::AccessFlagBits::eShaderWrite;
			access_pattern.queue_family_type = QueueFamilyTypes::eGraphics;
		}
		break;
		case BufferUsageTypes::eTransferDst:
		{
			// Buffer will be used as transfer destination
			access_pattern.stage             = vk::PipelineStageFlagBits::eTransfer;
			access_pattern.access_mask       = vk::AccessFlagBits::eTransferWrite;
			access_pattern.queue_family_type = QueueFamilyTypes::eTransfer;
		}
		break;
		case BufferUsageTypes::eTransferSrc:
		{
			// Buffer will be used as transfer source
			access_pattern.stage             = vk::PipelineStageFlagBits::eTransfer;
			access_pattern.access_mask       = vk::AccessFlags();
			access_pattern.queue_family_type = QueueFamilyTypes::eTransfer;
		}
		break;
		case BufferUsageTypes::eNone:
		{
			// Buffer not in use
			access_pattern.stage             = vk::PipelineStageFlagBits::eTopOfPipe;
			access_pattern.access_mask       = vk::AccessFlags();
			access_pattern.queue_family_type = QueueFamilyTypes::eUndefined;
		}
		break;
		case BufferUsageTypes::eUnknown:
		{
			// Buffer usage unknown
			access_pattern.stage             = vk::PipelineStageFlagBits::eBottomOfPipe;
			access_pattern.access_mask       = vk::AccessFlags();
			access_pattern.queue_family_type = QueueFamilyTypes::eUndefined;
		}
		break;
	};
	return access_pattern;
}

/**
 * @brief Get destination access pattern for a buffer
 * @param usage_type Buffer usage type
 * @return Destination access pattern
 */
static BufferAccessPattern get_dst_buffer_access_pattern(const BufferUsageTypes usage_type)
{
	BufferAccessPattern access_pattern;
	switch (usage_type)
	{
		case BufferUsageTypes::eVertexBuffer:
		{
			// Buffer used as vertex buffer
			access_pattern.stage             = vk::PipelineStageFlagBits::eVertexInput;
			access_pattern.access_mask       = vk::AccessFlagBits::eVertexAttributeRead;
			access_pattern.queue_family_type = QueueFamilyTypes::eGraphics;
		}
		break;
		case BufferUsageTypes::eGraphicsShaderReadWrite:
		{
			// Buffer will be read/written by graphics shader
			access_pattern.stage = vk::PipelineStageFlagBits::eVertexShader |
			                       vk::PipelineStageFlagBits::eFragmentShader;
			access_pattern.access_mask       = vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead;
			access_pattern.queue_family_type = QueueFamilyTypes::eGraphics;
		}
		break;
		case BufferUsageTypes::eComputeShaderReadWrite:
		{
			// Buffer will be read/written by compute shader
			access_pattern.stage             = vk::PipelineStageFlagBits::eComputeShader;
			access_pattern.access_mask       = vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead;
			access_pattern.queue_family_type = QueueFamilyTypes::eCompute;
		}
		break;
		case BufferUsageTypes::eIndirectBuffer:
		{
			// Buffer used for indirect commands (like visible_mesh_count_buffer in MeshShading)
			access_pattern.stage             = vk::PipelineStageFlagBits::eDrawIndirect;
			access_pattern.access_mask       = vk::AccessFlagBits::eIndirectCommandRead;
			access_pattern.queue_family_type = QueueFamilyTypes::eGraphics;
		}
		break;
		case BufferUsageTypes::eTransferDst:
		{
			// Buffer will be used as transfer destination
			access_pattern.stage             = vk::PipelineStageFlagBits::eTransfer;
			access_pattern.access_mask       = vk::AccessFlagBits::eTransferWrite;
			access_pattern.queue_family_type = QueueFamilyTypes::eTransfer;
		}
		break;
		case BufferUsageTypes::eTransferSrc:
		{
			// Buffer will be used as transfer source
			access_pattern.stage             = vk::PipelineStageFlagBits::eTransfer;
			access_pattern.access_mask       = vk::AccessFlagBits::eTransferRead;
			access_pattern.queue_family_type = QueueFamilyTypes::eTransfer;
		}
		break;
		case BufferUsageTypes::eNone:
		{
			// Buffer not in use
			access_pattern.stage             = vk::PipelineStageFlagBits::eBottomOfPipe;
			access_pattern.access_mask       = vk::AccessFlags();
			access_pattern.queue_family_type = QueueFamilyTypes::eUndefined;
		}
		break;
		case BufferUsageTypes::eUnknown:
		{
			// Buffer usage unknown
			access_pattern.stage             = vk::PipelineStageFlagBits::eBottomOfPipe;
			access_pattern.access_mask       = vk::AccessFlags();
			access_pattern.queue_family_type = QueueFamilyTypes::eUndefined;
		}
		break;
	};
	return access_pattern;
}

/**
 * @brief Determine if a barrier is needed between two buffer usage types
 * @param src_usage_type Source buffer usage type
 * @param dst_usage_type Destination buffer usage type
 * @return Returns true if a barrier is needed, otherwise false
 * @note Current implementation is conservative, always returns true
 */
static bool is_buffer_barrier_needed(const BufferUsageTypes src_usage_type, const BufferUsageTypes dst_usage_type)
{
	// could be smarter
	return true;
}
}        // namespace lz
