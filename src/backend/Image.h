#pragma once
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
		bool Contains(const ImageSubresourceRange& other)
		{
			return baseMipLevel <= other.baseMipLevel && baseArrayLayer <= other.baseArrayLayer && mipsCount >= other.
				mipsCount && arrayLayersCount >= other.arrayLayersCount;
		}

		// Comparison operator for container sorting
		bool operator <(const ImageSubresourceRange& other) const
		{
			return std::tie(baseMipLevel, mipsCount, baseArrayLayer, arrayLayersCount) < std::tie(
				other.baseMipLevel, other.mipsCount, other.baseArrayLayer, other.arrayLayersCount);
		}

		uint32_t baseMipLevel;      // First mipmap level in the range
		uint32_t mipsCount;         // Number of mipmap levels in the range
		uint32_t baseArrayLayer;    // First array layer in the range
		uint32_t arrayLayersCount;  // Number of array layers in the range
	};

	// ImageData: Class that holds information about a Vulkan image without owning the resource
	// - Contains image metadata and layout tracking for each mip level and array layer
	class ImageData
	{
	public:
		// GetHandle: Returns the native Vulkan image handle
		vk::Image GetHandle() const
		{
			return imageHandle;
		}

		// GetFormat: Returns the format of the image
		vk::Format GetFormat() const
		{
			return format;
		}

		// GetType: Returns the type of the image (1D, 2D, or 3D)
		vk::ImageType GetType() const
		{
			return imageType;
		}

		// GetMipSize: Returns the size of a specific mipmap level
		// Parameters:
		// - mipLevel: Mipmap level to query
		// Returns: Size of the specified mipmap level
		glm::uvec3 GetMipSize(uint32_t mipLevel)
		{
			return mipInfos[mipLevel].size;
		}

		// GetAspectFlags: Returns the aspect flags of the image (color or depth)
		vk::ImageAspectFlags GetAspectFlags() const
		{
			return aspectFlags;
		}

		uint32_t GetArrayLayersCount()
		{
			return arrayLayersCount;
		}

		uint32_t GetMipsCount()
		{
			return mipsCount;
		}

		bool operator <(const ImageData& other) const
		{
			return std::tie(imageHandle) < std::tie(other.imageHandle);
		}

	private:
		ImageData(vk::Image imageHandle, vk::ImageType imageType, glm::uvec3 size, uint32_t mipsCount,
		          uint32_t arrayLayersCount, vk::Format format, vk::ImageLayout layout)
		{
			this->imageHandle = imageHandle;
			this->format = format;
			this->mipsCount = mipsCount;
			this->arrayLayersCount = arrayLayersCount;
			this->imageType = imageType;

			glm::vec3 currSize = size;

			for (size_t mipLevel = 0; mipLevel < mipsCount; ++mipLevel)
			{
				MipInfo mipInfo;
				mipInfo.size = currSize;
				currSize.x /= 2;
				if (imageType == vk::ImageType::e2D || imageType == vk::ImageType::e3D)
				{
					currSize.y /= 2;
				}
				if (imageType == vk::ImageType::e3D)
				{
					currSize.z /= 2;
				}

				mipInfo.layerInfos.resize(arrayLayersCount);

				for (size_t layerIndex = 0; layerIndex < arrayLayersCount; ++layerIndex)
				{
					mipInfo.layerInfos[layerIndex].currLayout = layout;
				}
				mipInfos.push_back(mipInfo);
			}
			if (lz::IsDepthFormat(format))
			{
				this->aspectFlags = vk::ImageAspectFlagBits::eDepth;
			}
			else
			{
				this->aspectFlags = vk::ImageAspectFlagBits::eColor;
			}
		}

		void SetDebugName(std::string debugName)
		{
			this->debugName = debugName;
		}

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
		lz::ImageData* GetImageData()
		{
			return imageData.get();
		}

		vk::DeviceMemory GetMemory()
		{
			return imageMemory.get();
		}

		static vk::ImageCreateInfo CreateInfo2d(glm::uvec2 size, uint32_t mipsCount, uint32_t arrayLayersCount,
		                                        vk::Format format, vk::ImageUsageFlags usage)
		{
			auto layout = vk::ImageLayout::eUndefined;
			auto imageInfo = vk::ImageCreateInfo()
			                 .setImageType(vk::ImageType::e2D)
			                 .setExtent(vk::Extent3D(size.x, size.y, 1))
			                 .setMipLevels(mipsCount)
			                 .setArrayLayers(arrayLayersCount)
			                 .setFormat(format)
			                 .setInitialLayout(layout)
			                 .setUsage(usage)
			                 .setSharingMode(vk::SharingMode::eExclusive)
			                 .setSamples(vk::SampleCountFlagBits::e1)
			                 .setFlags(vk::ImageCreateFlags());

			return imageInfo;
		}

		// CreateInfoVolume: Creates an image create info structure for 3D volume images
		// Parameters:
		// - size: Width, height, and depth of the volume
		// - mipsCount: Number of mipmap levels
		// - arrayLayersCount: Number of array layers
		// - format: Format of the image
		// - usage: Usage flags for the image
		// Returns: Configured image create info structure for a 3D volume image
		static vk::ImageCreateInfo CreateInfoVolume(glm::uvec3 size, uint32_t mipsCount, uint32_t arrayLayersCount,
		                                            vk::Format format, vk::ImageUsageFlags usage)
		{
			auto layout = vk::ImageLayout::eUndefined;
			auto imageInfo = vk::ImageCreateInfo()
			                 .setImageType(vk::ImageType::e3D)
			                 .setExtent(vk::Extent3D(size.x, size.y, size.z))
			                 .setMipLevels(mipsCount)
			                 .setArrayLayers(arrayLayersCount)
			                 .setFormat(format)
			                 .setInitialLayout(layout) //images must be created in undefined layout
			                 .setUsage(usage)
			                 .setSharingMode(vk::SharingMode::eExclusive)
			                 .setSamples(vk::SampleCountFlagBits::e1)
			                 .setFlags(vk::ImageCreateFlags())
			                 .setTiling(vk::ImageTiling::eOptimal);
			return imageInfo;
		}

		// CreateInfoCube: Creates an image create info structure for cubemap images
		// Parameters:
		// - size: Width and height of each face of the cubemap
		// - mipsCount: Number of mipmap levels
		// - format: Format of the image
		// - usage: Usage flags for the image
		// Returns: Configured image create info structure for a cubemap
		static vk::ImageCreateInfo CreateInfoCube(glm::uvec2 size, uint32_t mipsCount, vk::Format format,
		                                          vk::ImageUsageFlags usage)
		{
			auto layout = vk::ImageLayout::eUndefined;
			auto imageInfo = vk::ImageCreateInfo()
			                 .setImageType(vk::ImageType::e2D)
			                 .setExtent(vk::Extent3D(size.x, size.y, 1))
			                 .setMipLevels(mipsCount)
			                 .setArrayLayers(6)  // Cubemap has 6 faces
			                 .setFormat(format)
			                 .setInitialLayout(layout)
			                 .setUsage(usage)
			                 .setSharingMode(vk::SharingMode::eExclusive)
			                 .setSamples(vk::SampleCountFlagBits::e1)
			                 .setFlags(vk::ImageCreateFlagBits::eCubeCompatible);

			return imageInfo;
		}

		// Constructor: Creates a new image with specified properties
		// Parameters:
		// - physicalDevice: Physical device for memory allocation
		// - logicalDevice: Logical device for image operations
		// - imageInfo: Image creation parameters
		// - memFlags: Memory property flags for the image allocation
		Image(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice, vk::ImageCreateInfo imageInfo,
		      vk::MemoryPropertyFlags memFlags = vk::MemoryPropertyFlagBits::eDeviceLocal)
		{
			// Create the image resource
			imageHandle = logicalDevice.createImageUnique(imageInfo);
			glm::uvec3 size = {imageInfo.extent.width, imageInfo.extent.height, imageInfo.extent.depth};

			// Create image data object to track image properties and layout
			imageData.reset(new lz::ImageData(imageHandle.get(), imageInfo.imageType, size, imageInfo.mipLevels,
			                                  imageInfo.arrayLayers, imageInfo.format, imageInfo.initialLayout));

			// Get memory requirements for the image
			vk::MemoryRequirements imageMemRequirements = logicalDevice.getImageMemoryRequirements(imageHandle.get());

			// Allocate memory for the image
			auto allocInfo = vk::MemoryAllocateInfo()
			                 .setAllocationSize(imageMemRequirements.size)
			                 .setMemoryTypeIndex(
				                 lz::FindMemoryTypeIndex(physicalDevice, imageMemRequirements.memoryTypeBits,
				                                         memFlags));

			imageMemory = logicalDevice.allocateMemoryUnique(allocInfo);

			// Bind the image to the allocated memory
			logicalDevice.bindImageMemory(imageHandle.get(), imageMemory.get(), 0);
		}

	private:
		vk::UniqueImage imageHandle;             // Native Vulkan image handle
		std::unique_ptr<lz::ImageData> imageData; // Image metadata and layout tracking
		vk::UniqueDeviceMemory imageMemory;      // Device memory allocation for this image
	};
}
