#pragma once

#include "LingzeVK.h"

namespace lz
{
	// QueueFamilyTypes: Enumeration of different Vulkan queue family types
	// - Used to identify which queue a command should be submitted to
	enum struct QueueFamilyTypes :uint8_t
	{
		eGraphics,  // Graphics queue family (vertex/fragment shaders, drawing)
		eTransfer,  // Transfer queue family (memory transfers, copies)
		eCompute,   // Compute queue family (compute shaders)
		ePresent,   // Presentation queue family (presenting to display)
		eUndefined  // Undefined queue family
	};

	// ImageAccessPattern: Structure describing how an image is accessed
	// - Contains information about pipeline stages, access flags, layout, and queue family
	struct ImageAccessPattern
	{
		vk::PipelineStageFlags stage;     // Pipeline stage where the image is accessed
		vk::AccessFlags access_mask;       // Memory access flags for the image
		vk::ImageLayout layout;           // Layout the image should be in
		QueueFamilyTypes queue_family_type; // Queue family that performs the access
	};

	// ImageSubresourceBarrier: Structure for barrier between two image access patterns
	// - Describes transition from one access pattern to another
	struct ImageSubresourceBarrier
	{
		ImageAccessPattern access_pattern;    // Source access pattern
		ImageAccessPattern dst_access_pattern; // Destination access pattern
	};

	// ImageUsageTypes: Enumeration of different ways an image can be used
	// - Used to identify how an image is being used at a particular point
	enum struct ImageUsageTypes : uint8_t
	{
		eGraphicsShaderRead,      // Read from a graphics shader (vertex/fragment)
		eGraphicsShaderReadWrite, // Read and write from a graphics shader
		eComputeShaderRead,       // Read from a compute shader
		eComputeShaderReadWrite,  // Read and write from a compute shader
		eTransferDst,             // Destination of a transfer operation
		eTransferSrc,             // Source of a transfer operation
		eColorAttachment,         // Used as a color attachment
		eDepthAttachment,         // Used as a depth attachment
		ePresent,                 // Used for presentation
		eNone,                    // Not used
		eUnknown                  // Usage is unknown or unspecified
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
				access_pattern.stage = vk::PipelineStageFlagBits::eFragmentShader;
				access_pattern.access_mask = vk::AccessFlags();
				access_pattern.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
				access_pattern.queue_family_type = QueueFamilyTypes::eGraphics;
			}
			break;
		case ImageUsageTypes::eGraphicsShaderReadWrite:
			{
				// Image was read/written by graphics shaders
				access_pattern.stage = vk::PipelineStageFlagBits::eVertexShader |
					vk::PipelineStageFlagBits::eFragmentShader;
				access_pattern.access_mask = vk::AccessFlagBits::eShaderWrite;
				access_pattern.layout = vk::ImageLayout::eGeneral;
				access_pattern.queue_family_type = QueueFamilyTypes::eGraphics;
			}
			break;
		case ImageUsageTypes::eComputeShaderRead:
			{
				// Image was read by compute shader
				access_pattern.stage = vk::PipelineStageFlagBits::eComputeShader;
				access_pattern.access_mask = vk::AccessFlags();
				access_pattern.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
				access_pattern.queue_family_type = QueueFamilyTypes::eCompute;
			}
			break;
		case ImageUsageTypes::eComputeShaderReadWrite:
			{
				// Image was read/written by compute shader
				access_pattern.stage = vk::PipelineStageFlagBits::eComputeShader;
				access_pattern.access_mask = vk::AccessFlagBits::eShaderWrite;
				access_pattern.layout = vk::ImageLayout::eGeneral;
				access_pattern.queue_family_type = QueueFamilyTypes::eCompute;
			}
			break;
		case ImageUsageTypes::eTransferSrc:
			{
				// Image was source of a transfer
				access_pattern.stage = vk::PipelineStageFlagBits::eTransfer;
				access_pattern.access_mask = vk::AccessFlags();
				access_pattern.layout = vk::ImageLayout::eTransferSrcOptimal;
				access_pattern.queue_family_type = QueueFamilyTypes::eTransfer;
			}
			break;
		case ImageUsageTypes::eTransferDst:
			{
				// Image was destination of a transfer
				access_pattern.stage = vk::PipelineStageFlagBits::eTransfer;
				access_pattern.access_mask = vk::AccessFlagBits::eTransferWrite;
				access_pattern.layout = vk::ImageLayout::eTransferDstOptimal;
				access_pattern.queue_family_type = QueueFamilyTypes::eTransfer;
			}
			break;
		case ImageUsageTypes::eColorAttachment:
			{
				// Image was used as a color attachment
				access_pattern.stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
				access_pattern.access_mask = vk::AccessFlagBits::eColorAttachmentWrite;
				access_pattern.layout = vk::ImageLayout::eColorAttachmentOptimal;
				access_pattern.queue_family_type = QueueFamilyTypes::eGraphics;
			}
			break;
		case ImageUsageTypes::eDepthAttachment:
			{
				// Image was used as a depth attachment
				access_pattern.stage = vk::PipelineStageFlagBits::eLateFragmentTests;
				access_pattern.access_mask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
				access_pattern.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
				access_pattern.queue_family_type = QueueFamilyTypes::eGraphics;
			}
			break;
		case ImageUsageTypes::ePresent:
			{
				// Image was used for presentation
				access_pattern.stage = vk::PipelineStageFlagBits::eBottomOfPipe;
				access_pattern.access_mask = vk::AccessFlags();
				access_pattern.layout = vk::ImageLayout::ePresentSrcKHR;
				access_pattern.queue_family_type = QueueFamilyTypes::ePresent;
			}
			break;
		case ImageUsageTypes::eNone:
			{
				// Image was not used
				access_pattern.stage = vk::PipelineStageFlagBits::eTopOfPipe;
				access_pattern.access_mask = vk::AccessFlags();
				access_pattern.layout = vk::ImageLayout::eUndefined;
				access_pattern.queue_family_type = QueueFamilyTypes::eUndefined;
			}
			break;
		case ImageUsageTypes::eUnknown:
			{
				// Image usage is unknown
				access_pattern.stage = vk::PipelineStageFlagBits::eBottomOfPipe;
				access_pattern.access_mask = vk::AccessFlags();
				access_pattern.layout = vk::ImageLayout::eUndefined;
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
				access_pattern.stage = vk::PipelineStageFlagBits::eVertexShader;
				access_pattern.access_mask = vk::AccessFlagBits::eShaderRead;
				access_pattern.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
				access_pattern.queue_family_type = QueueFamilyTypes::eGraphics;
			}
			break;
		case ImageUsageTypes::eGraphicsShaderReadWrite:
			{
				// Image will be read/written by graphics shaders
				access_pattern.stage = vk::PipelineStageFlagBits::eVertexShader |
					vk::PipelineStageFlagBits::eFragmentShader;
				access_pattern.access_mask = vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;
				access_pattern.layout = vk::ImageLayout::eGeneral;
				access_pattern.queue_family_type = QueueFamilyTypes::eGraphics;
			}
			break;
		case ImageUsageTypes::eComputeShaderRead:
			{
				// Image will be read by compute shader
				access_pattern.stage = vk::PipelineStageFlagBits::eComputeShader;
				access_pattern.access_mask = vk::AccessFlagBits::eShaderRead;
				access_pattern.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
				access_pattern.queue_family_type = QueueFamilyTypes::eCompute;
			}
			break;
		case ImageUsageTypes::eComputeShaderReadWrite:
			{
				// Image will be read/written by compute shader
				access_pattern.stage = vk::PipelineStageFlagBits::eComputeShader;
				access_pattern.access_mask = vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead;
				access_pattern.layout = vk::ImageLayout::eGeneral;
				access_pattern.queue_family_type = QueueFamilyTypes::eCompute;
			}
			break;
		case ImageUsageTypes::eTransferDst:
			{
				// Image will be destination of a transfer
				access_pattern.stage = vk::PipelineStageFlagBits::eTransfer;
				access_pattern.access_mask = vk::AccessFlagBits::eTransferWrite;
				access_pattern.layout = vk::ImageLayout::eTransferDstOptimal;
				access_pattern.queue_family_type = QueueFamilyTypes::eTransfer;
			}
			break;
		case ImageUsageTypes::eTransferSrc:
			{
				// Image will be source of a transfer
				access_pattern.stage = vk::PipelineStageFlagBits::eTransfer;
				access_pattern.access_mask = vk::AccessFlagBits::eTransferRead;
				access_pattern.layout = vk::ImageLayout::eTransferSrcOptimal;
				access_pattern.queue_family_type = QueueFamilyTypes::eTransfer;
			}
			break;
		case ImageUsageTypes::eColorAttachment:
			{
				// Image will be used as a color attachment
				access_pattern.stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
				access_pattern.access_mask = vk::AccessFlagBits::eColorAttachmentRead |
					vk::AccessFlagBits::eColorAttachmentWrite;
				access_pattern.layout = vk::ImageLayout::eColorAttachmentOptimal;
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
				access_pattern.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
				access_pattern.queue_family_type = QueueFamilyTypes::eGraphics;
			}
			break;
		case ImageUsageTypes::ePresent:
			{
				// Image will be used for presentation
				access_pattern.stage = vk::PipelineStageFlagBits::eBottomOfPipe;
				access_pattern.access_mask = vk::AccessFlags();
				access_pattern.layout = vk::ImageLayout::ePresentSrcKHR;
				access_pattern.queue_family_type = QueueFamilyTypes::ePresent;
			}
			break;
		case ImageUsageTypes::eNone:
			{
				// Image will not be used
				access_pattern.stage = vk::PipelineStageFlagBits::eBottomOfPipe;
				access_pattern.access_mask = vk::AccessFlags();
				access_pattern.layout = vk::ImageLayout::eUndefined;
				access_pattern.queue_family_type = QueueFamilyTypes::eUndefined;
			}
			break;
		case ImageUsageTypes::eUnknown:
			{
				// Image usage is unknown
				assert(0);
				access_pattern.stage = vk::PipelineStageFlagBits::eTopOfPipe;
				access_pattern.access_mask = vk::AccessFlags();
				access_pattern.layout = vk::ImageLayout::eUndefined;
				access_pattern.queue_family_type = QueueFamilyTypes::eUndefined;
			}
			break;
		};
		return access_pattern;
	}

	/**
	 * @brief 判断两个图像使用类型之间是否需要障碍(Barrier)
	 * @param src_usage_type 源图像使用类型
	 * @param dst_usage_type 目标图像使用类型
	 * @return 如果需要障碍返回true，否则返回false
	 */
	static bool is_image_barrier_needed(const ImageUsageTypes src_usage_type, const ImageUsageTypes dst_usage_type)
	{
		if (src_usage_type == ImageUsageTypes::eGraphicsShaderRead && dst_usage_type == ImageUsageTypes::eGraphicsShaderRead)
			return false;
		return true;
	}

	/**
	 * @brief 缓冲区访问模式
	 * 描述在管线的某个阶段如何访问缓冲区以及与之相关的访问掩码和队列类型
	 */
	struct BufferAccessPattern
	{
		vk::PipelineStageFlags stage;       // 访问缓冲区的管线阶段
		vk::AccessFlags access_mask;         // 内存访问标志
		QueueFamilyTypes queue_family_type;   // 执行访问的队列族类型
	};

	/**
	 * @brief 缓冲区障碍
	 * 描述缓冲区从一种访问模式转换到另一种访问模式所需的障碍信息
	 */
	struct BufferBarrier
	{
		ImageAccessPattern src_access_pattern;   // 源访问模式
		ImageAccessPattern dst_access_pattern;   // 目标访问模式
	};

	/**
	 * @brief 缓冲区使用类型枚举
	 * 定义了缓冲区在Vulkan管线中的不同使用方式
	 */
	enum struct BufferUsageTypes : uint8_t
	{
		eVertexBuffer,              // 用作顶点缓冲区
		eGraphicsShaderReadWrite,   // 被图形着色器读写
		eComputeShaderReadWrite,    // 被计算着色器读写
		eTransferDst,               // 作为传输操作的目标
		eTransferSrc,               // 作为传输操作的源
		eNone,                      // 不使用
		eUnknown                    // 未知用途
	};

	/**
	 * @brief 获取缓冲区的源访问模式
	 * @param usage_type 缓冲区使用类型
	 * @return 源访问模式
	 */
	static BufferAccessPattern get_src_buffer_access_pattern(const BufferUsageTypes usage_type)
	{
		BufferAccessPattern access_pattern;
		switch (usage_type)
		{
		case BufferUsageTypes::eVertexBuffer:
			{
				// 缓冲区作为顶点缓冲区使用
				access_pattern.stage = vk::PipelineStageFlagBits::eVertexInput;
				access_pattern.access_mask = vk::AccessFlags();
				access_pattern.queue_family_type = QueueFamilyTypes::eGraphics;
			}
			break;
		case BufferUsageTypes::eGraphicsShaderReadWrite:
			{
				// 缓冲区将由图形着色器读写
				access_pattern.stage = vk::PipelineStageFlagBits::eVertexShader |
					vk::PipelineStageFlagBits::eFragmentShader;
				access_pattern.access_mask = vk::AccessFlagBits::eShaderWrite;
				access_pattern.queue_family_type = QueueFamilyTypes::eGraphics;
			}
			break;
		case BufferUsageTypes::eComputeShaderReadWrite:
			{
				// 缓冲区将由计算着色器读写
				access_pattern.stage = vk::PipelineStageFlagBits::eComputeShader;
				access_pattern.access_mask = vk::AccessFlagBits::eShaderWrite;
				access_pattern.queue_family_type = QueueFamilyTypes::eCompute;
			}
			break;
		case BufferUsageTypes::eTransferDst:
			{
				// 缓冲区将作为传输目标
				access_pattern.stage = vk::PipelineStageFlagBits::eTransfer;
				access_pattern.access_mask = vk::AccessFlagBits::eTransferWrite;
				access_pattern.queue_family_type = QueueFamilyTypes::eTransfer;
			}
			break;
		case BufferUsageTypes::eTransferSrc:
			{
				// 缓冲区将作为传输源
				access_pattern.stage = vk::PipelineStageFlagBits::eTransfer;
				access_pattern.access_mask = vk::AccessFlags();
				access_pattern.queue_family_type = QueueFamilyTypes::eTransfer;
			}
			break;
		case BufferUsageTypes::eNone:
			{
				// 缓冲区不使用
				access_pattern.stage = vk::PipelineStageFlagBits::eTopOfPipe;
				access_pattern.access_mask = vk::AccessFlags();
				access_pattern.queue_family_type = QueueFamilyTypes::eUndefined;
			}
			break;
		case BufferUsageTypes::eUnknown:
			{
				// 缓冲区用途未知
				access_pattern.stage = vk::PipelineStageFlagBits::eBottomOfPipe;
				access_pattern.access_mask = vk::AccessFlags();
				access_pattern.queue_family_type = QueueFamilyTypes::eUndefined;
			}
			break;
		};
		return access_pattern;
	}

	/**
	 * @brief 获取缓冲区的目标访问模式
	 * @param usage_type 缓冲区使用类型
	 * @return 目标访问模式
	 */
	static BufferAccessPattern get_dst_buffer_access_pattern(const BufferUsageTypes usage_type)
	{
		BufferAccessPattern access_pattern;
		switch (usage_type)
		{
		case BufferUsageTypes::eVertexBuffer:
			{
				// 缓冲区作为顶点缓冲区使用
				access_pattern.stage = vk::PipelineStageFlagBits::eVertexInput;
				access_pattern.access_mask = vk::AccessFlagBits::eVertexAttributeRead;
				access_pattern.queue_family_type = QueueFamilyTypes::eGraphics;
			}
			break;
		case BufferUsageTypes::eGraphicsShaderReadWrite:
			{
				// 缓冲区将由图形着色器读写
				access_pattern.stage = vk::PipelineStageFlagBits::eVertexShader |
					vk::PipelineStageFlagBits::eFragmentShader;
				access_pattern.access_mask = vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead;
				access_pattern.queue_family_type = QueueFamilyTypes::eGraphics;
			}
			break;
		case BufferUsageTypes::eComputeShaderReadWrite:
			{
				// 缓冲区将由计算着色器读写
				access_pattern.stage = vk::PipelineStageFlagBits::eComputeShader;
				access_pattern.access_mask = vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead;
				access_pattern.queue_family_type = QueueFamilyTypes::eCompute;
			}
			break;
		case BufferUsageTypes::eTransferDst:
			{
				// 缓冲区将作为传输目标
				access_pattern.stage = vk::PipelineStageFlagBits::eTransfer;
				access_pattern.access_mask = vk::AccessFlagBits::eTransferWrite;
				access_pattern.queue_family_type = QueueFamilyTypes::eTransfer;
			}
			break;
		case BufferUsageTypes::eTransferSrc:
			{
				// 缓冲区将作为传输源
				access_pattern.stage = vk::PipelineStageFlagBits::eTransfer;
				access_pattern.access_mask = vk::AccessFlagBits::eTransferRead;
				access_pattern.queue_family_type = QueueFamilyTypes::eTransfer;
			}
			break;
		case BufferUsageTypes::eNone:
			{
				// 缓冲区不使用
				access_pattern.stage = vk::PipelineStageFlagBits::eBottomOfPipe;
				access_pattern.access_mask = vk::AccessFlags();
				access_pattern.queue_family_type = QueueFamilyTypes::eUndefined;
			}
			break;
		case BufferUsageTypes::eUnknown:
			{
				// 缓冲区用途未知
				access_pattern.stage = vk::PipelineStageFlagBits::eBottomOfPipe;
				access_pattern.access_mask = vk::AccessFlags();
				access_pattern.queue_family_type = QueueFamilyTypes::eUndefined;
			}
			break;
		};
		return access_pattern;
	}

	/**
	 * @brief 判断两个缓冲区使用类型之间是否需要障碍(Barrier)
	 * @param src_usage_type 源缓冲区使用类型
	 * @param dst_usage_type 目标缓冲区使用类型
	 * @return 如果需要障碍返回true，否则返回false
	 * @note 当前实现较为保守，总是返回true
	 */
	static bool is_buffer_barrier_needed(const BufferUsageTypes src_usage_type, const BufferUsageTypes dst_usage_type)
	{
		//could be smarter
		return true;
	}
}
