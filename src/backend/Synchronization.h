#pragma once

#include "LingzeVK.h"

namespace lz
{
	// QueueFamilyTypes: Enumeration of different Vulkan queue family types
	// - Used to identify which queue a command should be submitted to
	enum struct QueueFamilyTypes
	{
		Graphics,  // Graphics queue family (vertex/fragment shaders, drawing)
		Transfer,  // Transfer queue family (memory transfers, copies)
		Compute,   // Compute queue family (compute shaders)
		Present,   // Presentation queue family (presenting to display)
		Undefined  // Undefined queue family
	};

	// ImageAccessPattern: Structure describing how an image is accessed
	// - Contains information about pipeline stages, access flags, layout, and queue family
	struct ImageAccessPattern
	{
		vk::PipelineStageFlags stage;     // Pipeline stage where the image is accessed
		vk::AccessFlags accessMask;       // Memory access flags for the image
		vk::ImageLayout layout;           // Layout the image should be in
		QueueFamilyTypes queueFamilyType; // Queue family that performs the access
	};

	// ImageSubresourceBarrier: Structure for barrier between two image access patterns
	// - Describes transition from one access pattern to another
	struct ImageSubresourceBarrier
	{
		ImageAccessPattern accessPattern;    // Source access pattern
		ImageAccessPattern dstAccessPattern; // Destination access pattern
	};

	// ImageUsageTypes: Enumeration of different ways an image can be used
	// - Used to identify how an image is being used at a particular point
	enum struct ImageUsageTypes
	{
		GraphicsShaderRead,      // Read from a graphics shader (vertex/fragment)
		GraphicsShaderReadWrite, // Read and write from a graphics shader
		ComputeShaderRead,       // Read from a compute shader
		ComputeShaderReadWrite,  // Read and write from a compute shader
		TransferDst,             // Destination of a transfer operation
		TransferSrc,             // Source of a transfer operation
		ColorAttachment,         // Used as a color attachment
		DepthAttachment,         // Used as a depth attachment
		Present,                 // Used for presentation
		None,                    // Not used
		Unknown                  // Usage is unknown or unspecified
	};

	// GetSrcImageAccessPattern: Creates an access pattern for the source of an image barrier
	// Parameters:
	// - usageTypes: How the image is used at the source of the barrier
	// Returns: Appropriate access pattern for the specified usage
	static ImageAccessPattern GetSrcImageAccessPattern(ImageUsageTypes usageTypes)
	{
		ImageAccessPattern accessPattern;
		switch (usageTypes)
		{
		case ImageUsageTypes::GraphicsShaderRead:
			{
				// Image was read by fragment shader
				accessPattern.stage = vk::PipelineStageFlagBits::eFragmentShader;
				accessPattern.accessMask = vk::AccessFlags();
				accessPattern.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
				accessPattern.queueFamilyType = QueueFamilyTypes::Graphics;
			}
			break;
		case ImageUsageTypes::GraphicsShaderReadWrite:
			{
				// Image was read/written by graphics shaders
				accessPattern.stage = vk::PipelineStageFlagBits::eVertexShader |
					vk::PipelineStageFlagBits::eFragmentShader;
				accessPattern.accessMask = vk::AccessFlagBits::eShaderWrite;
				accessPattern.layout = vk::ImageLayout::eGeneral;
				accessPattern.queueFamilyType = QueueFamilyTypes::Graphics;
			}
			break;
		case ImageUsageTypes::ComputeShaderRead:
			{
				// Image was read by compute shader
				accessPattern.stage = vk::PipelineStageFlagBits::eComputeShader;
				accessPattern.accessMask = vk::AccessFlags();
				accessPattern.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
				accessPattern.queueFamilyType = QueueFamilyTypes::Compute;
			}
			break;
		case ImageUsageTypes::ComputeShaderReadWrite:
			{
				// Image was read/written by compute shader
				accessPattern.stage = vk::PipelineStageFlagBits::eComputeShader;
				accessPattern.accessMask = vk::AccessFlagBits::eShaderWrite;
				accessPattern.layout = vk::ImageLayout::eGeneral;
				accessPattern.queueFamilyType = QueueFamilyTypes::Compute;
			}
			break;
		case ImageUsageTypes::TransferSrc:
			{
				// Image was source of a transfer
				accessPattern.stage = vk::PipelineStageFlagBits::eTransfer;
				accessPattern.accessMask = vk::AccessFlags();
				accessPattern.layout = vk::ImageLayout::eTransferSrcOptimal;
				accessPattern.queueFamilyType = QueueFamilyTypes::Transfer;
			}
			break;
		case ImageUsageTypes::TransferDst:
			{
				// Image was destination of a transfer
				accessPattern.stage = vk::PipelineStageFlagBits::eTransfer;
				accessPattern.accessMask = vk::AccessFlagBits::eTransferWrite;
				accessPattern.layout = vk::ImageLayout::eTransferDstOptimal;
				accessPattern.queueFamilyType = QueueFamilyTypes::Transfer;
			}
			break;
		case ImageUsageTypes::ColorAttachment:
			{
				// Image was used as a color attachment
				accessPattern.stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
				accessPattern.accessMask = vk::AccessFlagBits::eColorAttachmentWrite;
				accessPattern.layout = vk::ImageLayout::eColorAttachmentOptimal;
				accessPattern.queueFamilyType = QueueFamilyTypes::Graphics;
			}
			break;
		case ImageUsageTypes::DepthAttachment:
			{
				// Image was used as a depth attachment
				accessPattern.stage = vk::PipelineStageFlagBits::eLateFragmentTests;
				accessPattern.accessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
				accessPattern.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
				accessPattern.queueFamilyType = QueueFamilyTypes::Graphics;
			}
			break;
		case ImageUsageTypes::Present:
			{
				// Image was used for presentation
				accessPattern.stage = vk::PipelineStageFlagBits::eBottomOfPipe;
				accessPattern.accessMask = vk::AccessFlags();
				accessPattern.layout = vk::ImageLayout::ePresentSrcKHR;
				accessPattern.queueFamilyType = QueueFamilyTypes::Present;
			}
			break;
		case ImageUsageTypes::None:
			{
				// Image was not used
				accessPattern.stage = vk::PipelineStageFlagBits::eTopOfPipe;
				accessPattern.accessMask = vk::AccessFlags();
				accessPattern.layout = vk::ImageLayout::eUndefined;
				accessPattern.queueFamilyType = QueueFamilyTypes::Undefined;
			}
			break;
		case ImageUsageTypes::Unknown:
			{
				// Image usage is unknown
				accessPattern.stage = vk::PipelineStageFlagBits::eBottomOfPipe;
				accessPattern.accessMask = vk::AccessFlags();
				accessPattern.layout = vk::ImageLayout::eUndefined;
				accessPattern.queueFamilyType = QueueFamilyTypes::Undefined;
			}
			break;
		};
		return accessPattern;
	}

	// GetDstImageAccessPattern: Creates an access pattern for the destination of an image barrier
	// Parameters:
	// - usageType: How the image will be used after the barrier
	// Returns: Appropriate access pattern for the specified usage
	static ImageAccessPattern GetDstImageAccessPattern(ImageUsageTypes usageType)
	{
		ImageAccessPattern accessPattern;
		switch (usageType)
		{
		case ImageUsageTypes::GraphicsShaderRead:
			{
				// Image will be read by vertex shader
				accessPattern.stage = vk::PipelineStageFlagBits::eVertexShader;
				accessPattern.accessMask = vk::AccessFlagBits::eShaderRead;
				accessPattern.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
				accessPattern.queueFamilyType = QueueFamilyTypes::Graphics;
			}
			break;
		case ImageUsageTypes::GraphicsShaderReadWrite:
			{
				// Image will be read/written by graphics shaders
				accessPattern.stage = vk::PipelineStageFlagBits::eVertexShader |
					vk::PipelineStageFlagBits::eFragmentShader;
				accessPattern.accessMask = vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;
				accessPattern.layout = vk::ImageLayout::eGeneral;
				accessPattern.queueFamilyType = QueueFamilyTypes::Graphics;
			}
			break;
		case ImageUsageTypes::ComputeShaderRead:
			{
				// Image will be read by compute shader
				accessPattern.stage = vk::PipelineStageFlagBits::eComputeShader;
				accessPattern.accessMask = vk::AccessFlagBits::eShaderRead;
				accessPattern.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
				accessPattern.queueFamilyType = QueueFamilyTypes::Compute;
			}
			break;
		case ImageUsageTypes::ComputeShaderReadWrite:
			{
				// Image will be read/written by compute shader
				accessPattern.stage = vk::PipelineStageFlagBits::eComputeShader;
				accessPattern.accessMask = vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead;
				accessPattern.layout = vk::ImageLayout::eGeneral;
				accessPattern.queueFamilyType = QueueFamilyTypes::Compute;
			}
			break;
		case ImageUsageTypes::TransferDst:
			{
				// Image will be destination of a transfer
				accessPattern.stage = vk::PipelineStageFlagBits::eTransfer;
				accessPattern.accessMask = vk::AccessFlagBits::eTransferWrite;
				accessPattern.layout = vk::ImageLayout::eTransferDstOptimal;
				accessPattern.queueFamilyType = QueueFamilyTypes::Transfer;
			}
			break;
		case ImageUsageTypes::TransferSrc:
			{
				// Image will be source of a transfer
				accessPattern.stage = vk::PipelineStageFlagBits::eTransfer;
				accessPattern.accessMask = vk::AccessFlagBits::eTransferRead;
				accessPattern.layout = vk::ImageLayout::eTransferSrcOptimal;
				accessPattern.queueFamilyType = QueueFamilyTypes::Transfer;
			}
			break;
		case ImageUsageTypes::ColorAttachment:
			{
				// Image will be used as a color attachment
				accessPattern.stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
				accessPattern.accessMask = vk::AccessFlagBits::eColorAttachmentRead |
					vk::AccessFlagBits::eColorAttachmentWrite;
				accessPattern.layout = vk::ImageLayout::eColorAttachmentOptimal;
				accessPattern.queueFamilyType = QueueFamilyTypes::Graphics;
			}
			break;
		case ImageUsageTypes::DepthAttachment:
			{
				// Image will be used as a depth attachment
				accessPattern.stage = vk::PipelineStageFlagBits::eLateFragmentTests |
					vk::PipelineStageFlagBits::eEarlyFragmentTests;
				accessPattern.accessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead |
					vk::AccessFlagBits::eDepthStencilAttachmentWrite;
				accessPattern.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
				accessPattern.queueFamilyType = QueueFamilyTypes::Graphics;
			}
			break;
		case ImageUsageTypes::Present:
			{
				// Image will be used for presentation
				accessPattern.stage = vk::PipelineStageFlagBits::eBottomOfPipe;
				accessPattern.accessMask = vk::AccessFlags();
				accessPattern.layout = vk::ImageLayout::ePresentSrcKHR;
				accessPattern.queueFamilyType = QueueFamilyTypes::Present;
			}
			break;
		case ImageUsageTypes::None:
			{
				// Image will not be used
				accessPattern.stage = vk::PipelineStageFlagBits::eBottomOfPipe;
				accessPattern.accessMask = vk::AccessFlags();
				accessPattern.layout = vk::ImageLayout::eUndefined;
				accessPattern.queueFamilyType = QueueFamilyTypes::Undefined;
			}
			break;
		case ImageUsageTypes::Unknown:
			{
				// Image usage is unknown
				assert(0);
				accessPattern.stage = vk::PipelineStageFlagBits::eTopOfPipe;
				accessPattern.accessMask = vk::AccessFlags();
				accessPattern.layout = vk::ImageLayout::eUndefined;
				accessPattern.queueFamilyType = QueueFamilyTypes::Undefined;
			}
			break;
		};
		return accessPattern;
	}

	/**
	 * @brief 判断两个图像使用类型之间是否需要障碍(Barrier)
	 * @param srcUsageType 源图像使用类型
	 * @param dstUsageType 目标图像使用类型
	 * @return 如果需要障碍返回true，否则返回false
	 */
	static bool IsImageBarrierNeeded(ImageUsageTypes srcUsageType, ImageUsageTypes dstUsageType)
	{
		if (srcUsageType == ImageUsageTypes::GraphicsShaderRead && dstUsageType == ImageUsageTypes::GraphicsShaderRead)
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
		vk::AccessFlags accessMask;         // 内存访问标志
		QueueFamilyTypes queueFamilyType;   // 执行访问的队列族类型
	};

	/**
	 * @brief 缓冲区障碍
	 * 描述缓冲区从一种访问模式转换到另一种访问模式所需的障碍信息
	 */
	struct BufferBarrier
	{
		ImageAccessPattern srcAccessPattern;   // 源访问模式
		ImageAccessPattern dstAccessPattern;   // 目标访问模式
	};

	/**
	 * @brief 缓冲区使用类型枚举
	 * 定义了缓冲区在Vulkan管线中的不同使用方式
	 */
	enum struct BufferUsageTypes
	{
		VertexBuffer,              // 用作顶点缓冲区
		GraphicsShaderReadWrite,   // 被图形着色器读写
		ComputeShaderReadWrite,    // 被计算着色器读写
		TransferDst,               // 作为传输操作的目标
		TransferSrc,               // 作为传输操作的源
		None,                      // 不使用
		Unknown                    // 未知用途
	};

	/**
	 * @brief 获取缓冲区的源访问模式
	 * @param usageType 缓冲区使用类型
	 * @return 源访问模式
	 */
	static BufferAccessPattern GetSrcBufferAccessPattern(BufferUsageTypes usageType)
	{
		BufferAccessPattern accessPattern;
		switch (usageType)
		{
		case BufferUsageTypes::VertexBuffer:
			{
				// 缓冲区作为顶点缓冲区使用
				accessPattern.stage = vk::PipelineStageFlagBits::eVertexInput;
				accessPattern.accessMask = vk::AccessFlags();
				accessPattern.queueFamilyType = QueueFamilyTypes::Graphics;
			}
			break;
		case BufferUsageTypes::GraphicsShaderReadWrite:
			{
				// 缓冲区将由图形着色器读写
				accessPattern.stage = vk::PipelineStageFlagBits::eVertexShader |
					vk::PipelineStageFlagBits::eFragmentShader;
				accessPattern.accessMask = vk::AccessFlagBits::eShaderWrite;
				accessPattern.queueFamilyType = QueueFamilyTypes::Graphics;
			}
			break;
		case BufferUsageTypes::ComputeShaderReadWrite:
			{
				// 缓冲区将由计算着色器读写
				accessPattern.stage = vk::PipelineStageFlagBits::eComputeShader;
				accessPattern.accessMask = vk::AccessFlagBits::eShaderWrite;
				accessPattern.queueFamilyType = QueueFamilyTypes::Compute;
			}
			break;
		case BufferUsageTypes::TransferDst:
			{
				// 缓冲区将作为传输目标
				accessPattern.stage = vk::PipelineStageFlagBits::eTransfer;
				accessPattern.accessMask = vk::AccessFlagBits::eTransferWrite;
				accessPattern.queueFamilyType = QueueFamilyTypes::Transfer;
			}
			break;
		case BufferUsageTypes::TransferSrc:
			{
				// 缓冲区将作为传输源
				accessPattern.stage = vk::PipelineStageFlagBits::eTransfer;
				accessPattern.accessMask = vk::AccessFlags();
				accessPattern.queueFamilyType = QueueFamilyTypes::Transfer;
			}
			break;
		case BufferUsageTypes::None:
			{
				// 缓冲区不使用
				accessPattern.stage = vk::PipelineStageFlagBits::eTopOfPipe;
				accessPattern.accessMask = vk::AccessFlags();
				accessPattern.queueFamilyType = QueueFamilyTypes::Undefined;
			}
			break;
		case BufferUsageTypes::Unknown:
			{
				// 缓冲区用途未知
				accessPattern.stage = vk::PipelineStageFlagBits::eBottomOfPipe;
				accessPattern.accessMask = vk::AccessFlags();
				accessPattern.queueFamilyType = QueueFamilyTypes::Undefined;
			}
			break;
		};
		return accessPattern;
	}

	/**
	 * @brief 获取缓冲区的目标访问模式
	 * @param usageType 缓冲区使用类型
	 * @return 目标访问模式
	 */
	static BufferAccessPattern GetDstBufferAccessPattern(BufferUsageTypes usageType)
	{
		BufferAccessPattern accessPattern;
		switch (usageType)
		{
		case BufferUsageTypes::VertexBuffer:
			{
				// 缓冲区作为顶点缓冲区使用
				accessPattern.stage = vk::PipelineStageFlagBits::eVertexInput;
				accessPattern.accessMask = vk::AccessFlagBits::eVertexAttributeRead;
				accessPattern.queueFamilyType = QueueFamilyTypes::Graphics;
			}
			break;
		case BufferUsageTypes::GraphicsShaderReadWrite:
			{
				// 缓冲区将由图形着色器读写
				accessPattern.stage = vk::PipelineStageFlagBits::eVertexShader |
					vk::PipelineStageFlagBits::eFragmentShader;
				accessPattern.accessMask = vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead;
				accessPattern.queueFamilyType = QueueFamilyTypes::Graphics;
			}
			break;
		case BufferUsageTypes::ComputeShaderReadWrite:
			{
				// 缓冲区将由计算着色器读写
				accessPattern.stage = vk::PipelineStageFlagBits::eComputeShader;
				accessPattern.accessMask = vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead;
				accessPattern.queueFamilyType = QueueFamilyTypes::Compute;
			}
			break;
		case BufferUsageTypes::TransferDst:
			{
				// 缓冲区将作为传输目标
				accessPattern.stage = vk::PipelineStageFlagBits::eTransfer;
				accessPattern.accessMask = vk::AccessFlagBits::eTransferWrite;
				accessPattern.queueFamilyType = QueueFamilyTypes::Transfer;
			}
			break;
		case BufferUsageTypes::TransferSrc:
			{
				// 缓冲区将作为传输源
				accessPattern.stage = vk::PipelineStageFlagBits::eTransfer;
				accessPattern.accessMask = vk::AccessFlagBits::eTransferRead;
				accessPattern.queueFamilyType = QueueFamilyTypes::Transfer;
			}
			break;
		case BufferUsageTypes::None:
			{
				// 缓冲区不使用
				accessPattern.stage = vk::PipelineStageFlagBits::eBottomOfPipe;
				accessPattern.accessMask = vk::AccessFlags();
				accessPattern.queueFamilyType = QueueFamilyTypes::Undefined;
			}
			break;
		case BufferUsageTypes::Unknown:
			{
				// 缓冲区用途未知
				accessPattern.stage = vk::PipelineStageFlagBits::eBottomOfPipe;
				accessPattern.accessMask = vk::AccessFlags();
				accessPattern.queueFamilyType = QueueFamilyTypes::Undefined;
			}
			break;
		};
		return accessPattern;
	}

	/**
	 * @brief 判断两个缓冲区使用类型之间是否需要障碍(Barrier)
	 * @param srcUsageType 源缓冲区使用类型
	 * @param dstUsageType 目标缓冲区使用类型
	 * @return 如果需要障碍返回true，否则返回false
	 * @note 当前实现较为保守，总是返回true
	 */
	static bool IsBufferBarrierNeeded(BufferUsageTypes srcUsageType, BufferUsageTypes dstUsageType)
	{
		//could be smarter
		return true;
	}
}
