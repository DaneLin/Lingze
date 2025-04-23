#pragma once

#include "LingzeVK.h"

namespace lz
{
	// IsDepthFormat: Checks if a format is a depth format
	// Parameters:
	// - format: Vulkan format to check
	// Returns: True if the format is a depth format, false otherwise
	static bool IsDepthFormat(vk::Format format)
	{
		return (format >= vk::Format::eD16Unorm && format < vk::Format::eD32SfloatS8Uint);
	}

	// GetGeneralUsageFlags: Determines appropriate usage flags for an image based on its format
	// Parameters:
	// - format: Vulkan format of the image
	// Returns: Appropriate image usage flags for the given format
	static vk::ImageUsageFlags GetGeneralUsageFlags(vk::Format format)
	{
		vk::ImageUsageFlags usageFlags = vk::ImageUsageFlagBits::eSampled;
		if (IsDepthFormat(format))
		{
			usageFlags |= vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled;
		}
		else
		{
			usageFlags |= vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst |
				vk::ImageUsageFlagBits::eSampled;
		}
		return usageFlags;
	}

	// Common image usage flag combinations
	static const vk::ImageUsageFlags colorImageUsage = vk::ImageUsageFlagBits::eColorAttachment |
		vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
	static const vk::ImageUsageFlags depthImageUsage = vk::ImageUsageFlagBits::eDepthStencilAttachment |
		vk::ImageUsageFlagBits::eSampled;

	class Swapchain;
	class RenderTarget;
	class Image;

	// ImageSubresourceRange: Represents a range of mipmap levels and array layers within an image
	struct ImageSubresourceRange
	{
		// Contains: Checks if this range contains another range
		// Parameters:
		// - other: The range to check against
		// Returns: True if this range fully contains the other range
		bool Contains(const ImageSubresourceRange& other);

		// Comparison operator for container sorting
		bool operator <(const ImageSubresourceRange& other) const;

		uint32_t baseMipLevel; // First mipmap level in the range
		uint32_t mipsCount; // Number of mipmap levels in the range
		uint32_t baseArrayLayer; // First array layer in the range
		uint32_t arrayLayersCount; // Number of array layers in the range
	};


	// ImageData: Class that holds information about a Vulkan image without owning the resource
	// - Contains image metadata and layout tracking for each mip level and array layer
	class ImageData
	{
	public:
		// GetHandle: Returns the native Vulkan image handle
		vk::Image GetHandle() const;

		// GetFormat: Returns the format of the image
		vk::Format GetFormat() const;

		// GetType: Returns the type of the image (1D, 2D, or 3D)
		vk::ImageType GetType() const;

		// GetMipSize: Returns the size of a specific mipmap level
		// Parameters:
		// - mipLevel: Mipmap level to query
		// Returns: Size of the specified mipmap level
		glm::uvec3 GetMipSize(uint32_t mipLevel);

		// GetAspectFlags: Returns the aspect flags of the image (color or depth)
		vk::ImageAspectFlags GetAspectFlags() const;

		uint32_t GetArrayLayersCount();

		uint32_t GetMipsCount();

		bool operator <(const ImageData& other) const;

	private:
		ImageData(vk::Image imageHandle, vk::ImageType imageType, glm::uvec3 size, uint32_t mipsCount,
		          uint32_t arrayLayersCount, vk::Format format, vk::ImageLayout layout);

		void SetDebugName(std::string debugName);

		struct SubImageInfo
		{
			vk::ImageLayout currLayout;
		};

		struct MipInfo
		{
			std::vector<SubImageInfo> layerInfos;
			glm::uvec3 size;
		};

		std::vector<MipInfo> mipInfos;

		vk::ImageAspectFlags aspectFlags;
		vk::Image imageHandle;
		vk::Format format;
		vk::ImageType imageType;
		uint32_t mipsCount;
		uint32_t arrayLayersCount;
		std::string debugName;

		friend class lz::Image;
		friend class lz::Swapchain;
		friend class Core;
	};


	class Image
	{
	public:
		lz::ImageData* GetImageData();

		vk::DeviceMemory GetMemory();

		static vk::ImageCreateInfo CreateInfo2d(glm::uvec2 size, uint32_t mipsCount, uint32_t arrayLayersCount,
		                                        vk::Format format, vk::ImageUsageFlags usage);

		// CreateInfoVolume: Creates an image create info structure for 3D volume images
		// Parameters:
		// - size: Width, height, and depth of the volume
		// - mipsCount: Number of mipmap levels
		// - arrayLayersCount: Number of array layers
		// - format: Format of the image
		// - usage: Usage flags for the image
		// Returns: Configured image create info structure for a 3D volume image
		static vk::ImageCreateInfo CreateInfoVolume(glm::uvec3 size, uint32_t mipsCount, uint32_t arrayLayersCount,
		                                            vk::Format format, vk::ImageUsageFlags usage);

		// CreateInfoCube: Creates an image create info structure for cubemap images
		// Parameters:
		// - size: Width and height of each face of the cubemap
		// - mipsCount: Number of mipmap levels
		// - format: Format of the image
		// - usage: Usage flags for the image
		// Returns: Configured image create info structure for a cubemap
		static vk::ImageCreateInfo CreateInfoCube(glm::uvec2 size, uint32_t mipsCount, vk::Format format,
		                                          vk::ImageUsageFlags usage);

		// Constructor: Creates a new image with specified properties
		// Parameters:
		// - physicalDevice: Physical device for memory allocation
		// - logicalDevice: Logical device for image operations
		// - imageInfo: Image creation parameters
		// - memFlags: Memory property flags for the image allocation
		Image(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice, vk::ImageCreateInfo imageInfo,
		      vk::MemoryPropertyFlags memFlags = vk::MemoryPropertyFlagBits::eDeviceLocal);

	private:
		vk::UniqueImage imageHandle; // Native Vulkan image handle
		std::unique_ptr<lz::ImageData> imageData; // Image metadata and layout tracking
		vk::UniqueDeviceMemory imageMemory; // Device memory allocation for this image
	};
}
