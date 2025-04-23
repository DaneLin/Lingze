#include "Image.h"

#include "Buffer.h"

namespace lz
{
	bool ImageSubresourceRange::Contains(const ImageSubresourceRange& other)
	{
		return baseMipLevel <= other.baseMipLevel && baseArrayLayer <= other.baseArrayLayer && mipsCount >= other.
			mipsCount && arrayLayersCount >= other.arrayLayersCount;
	}

	bool ImageSubresourceRange::operator<(const ImageSubresourceRange& other) const
	{
		return std::tie(baseMipLevel, mipsCount, baseArrayLayer, arrayLayersCount) < std::tie(
			other.baseMipLevel, other.mipsCount, other.baseArrayLayer, other.arrayLayersCount);
	}

	vk::Image ImageData::GetHandle() const
	{
		return imageHandle;
	}

	vk::Format ImageData::GetFormat() const
	{
		return format;
	}

	vk::ImageType ImageData::GetType() const
	{
		return imageType;
	}

	glm::uvec3 ImageData::GetMipSize(uint32_t mipLevel)
	{
		return mipInfos[mipLevel].size;
	}

	vk::ImageAspectFlags ImageData::GetAspectFlags() const
	{
		return aspectFlags;
	}

	uint32_t ImageData::GetArrayLayersCount()
	{
		return arrayLayersCount;
	}

	uint32_t ImageData::GetMipsCount()
	{
		return mipsCount;
	}

	bool ImageData::operator<(const ImageData& other) const
	{
		return std::tie(imageHandle) < std::tie(other.imageHandle);
	}

	ImageData::ImageData(vk::Image imageHandle, vk::ImageType imageType, glm::uvec3 size, uint32_t mipsCount,
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

	void ImageData::SetDebugName(std::string debugName)
	{
		this->debugName = debugName;
	}


	lz::ImageData* Image::GetImageData()
	{
		return imageData.get();
	}

	vk::DeviceMemory Image::GetMemory()
	{
		return imageMemory.get();
	}

	vk::ImageCreateInfo Image::CreateInfo2d(glm::uvec2 size, uint32_t mipsCount, uint32_t arrayLayersCount,
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

	vk::ImageCreateInfo Image::CreateInfoVolume(glm::uvec3 size, uint32_t mipsCount, uint32_t arrayLayersCount,
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

	vk::ImageCreateInfo Image::CreateInfoCube(glm::uvec2 size, uint32_t mipsCount, vk::Format format,
	                                          vk::ImageUsageFlags usage)
	{
		auto layout = vk::ImageLayout::eUndefined;
		auto imageInfo = vk::ImageCreateInfo()
		                 .setImageType(vk::ImageType::e2D)
		                 .setExtent(vk::Extent3D(size.x, size.y, 1))
		                 .setMipLevels(mipsCount)
		                 .setArrayLayers(6) // Cubemap has 6 faces
		                 .setFormat(format)
		                 .setInitialLayout(layout)
		                 .setUsage(usage)
		                 .setSharingMode(vk::SharingMode::eExclusive)
		                 .setSamples(vk::SampleCountFlagBits::e1)
		                 .setFlags(vk::ImageCreateFlagBits::eCubeCompatible);

		return imageInfo;
	}

	Image::Image(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice, vk::ImageCreateInfo imageInfo,
	             vk::MemoryPropertyFlags memFlags)
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
			                 lz::find_memory_type_index(physicalDevice, imageMemRequirements.memoryTypeBits,
			                                            memFlags));

		imageMemory = logicalDevice.allocateMemoryUnique(allocInfo);

		// Bind the image to the allocated memory
		logicalDevice.bindImageMemory(imageHandle.get(), imageMemory.get(), 0);
	}
}
