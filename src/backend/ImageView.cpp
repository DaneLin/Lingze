#include "ImageView.h"

#include "Image.h"

namespace lz
{
	vk::ImageView ImageView::GetHandle() const
	{
		return imageView.get();
	}

	lz::ImageData* ImageView::GetImageData()
	{
		return imageData;
	}

	const lz::ImageData* ImageView::GetImageData() const
	{
		return imageData;
	}

	uint32_t ImageView::GetBaseMipLevel()
	{
		return baseMipLevel;
	}

	uint32_t ImageView::GetMipLevelsCount()
	{
		return mipLevelsCount;
	}

	uint32_t ImageView::GetBaseArrayLayer()
	{
		return baseArrayLayer;
	}

	uint32_t ImageView::GetArrayLayersCount()
	{
		return arrayLayersCount;
	}

	ImageView::ImageView(vk::Device logicalDevice, lz::ImageData* imageData, uint32_t baseMipLevel,
	                     uint32_t mipLevelsCount, uint32_t baseArrayLayer, uint32_t arrayLayersCount)
	{
		this->imageData = imageData;
		this->baseMipLevel = baseMipLevel;
		this->mipLevelsCount = mipLevelsCount;
		this->baseArrayLayer = baseArrayLayer;
		this->arrayLayersCount = arrayLayersCount;

		vk::Format format = imageData->GetFormat();
		vk::ImageAspectFlags aspectFlags;

		// Configure subresource range
		auto subresourceRange = vk::ImageSubresourceRange()
		                        .setAspectMask(imageData->GetAspectFlags())
		                        .setBaseMipLevel(baseMipLevel)
		                        .setLevelCount(mipLevelsCount)
		                        .setBaseArrayLayer(baseArrayLayer)
		                        .setLayerCount(arrayLayersCount);

		// Determine view type based on image type
		vk::ImageViewType viewType;
		if (imageData->GetType() == vk::ImageType::e1D)
			viewType = vk::ImageViewType::e1D;
		if (imageData->GetType() == vk::ImageType::e2D)
			viewType = vk::ImageViewType::e2D;
		if (imageData->GetType() == vk::ImageType::e3D)
			viewType = vk::ImageViewType::e3D;

		// Create the image view
		auto imageViewCreateInfo = vk::ImageViewCreateInfo()
		                           .setImage(imageData->GetHandle())
		                           .setViewType(viewType)
		                           .setFormat(format)
		                           .setSubresourceRange(subresourceRange);
		imageView = logicalDevice.createImageViewUnique(imageViewCreateInfo);
	}

	ImageView::ImageView(vk::Device logicalDevice, lz::ImageData* cubemapImageData, uint32_t baseMipLevel,
	                     uint32_t mipLevelsCount)
	{
		this->imageData = cubemapImageData;
		this->baseMipLevel = baseMipLevel;
		this->mipLevelsCount = mipLevelsCount;
		this->baseArrayLayer = 0;
		this->arrayLayersCount = 6;

		vk::Format format = imageData->GetFormat();
		vk::ImageAspectFlags aspectFlags;

		// Configure subresource range for cubemap (always 6 layers)
		auto subresourceRange = vk::ImageSubresourceRange()
		                        .setAspectMask(imageData->GetAspectFlags())
		                        .setBaseMipLevel(baseMipLevel)
		                        .setLevelCount(mipLevelsCount)
		                        .setBaseArrayLayer(baseArrayLayer)
		                        .setLayerCount(arrayLayersCount);

		// Cubemap view type
		vk::ImageViewType viewType = vk::ImageViewType::eCube;

		// Validation checks for cubemap
		assert(imageData->GetType() == vk::ImageType::e2D);
		assert(imageData->GetArrayLayersCount() == 6);

		// Create the cubemap image view
		auto imageViewCreateInfo = vk::ImageViewCreateInfo()
		                           .setImage(imageData->GetHandle())
		                           .setViewType(viewType)
		                           .setFormat(format)
		                           .setSubresourceRange(subresourceRange);
		imageView = logicalDevice.createImageViewUnique(imageViewCreateInfo);
	}
}
