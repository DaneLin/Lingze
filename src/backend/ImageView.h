#pragma once

namespace lz
{
	class Swapchain;
	class RenderTarget;

	// ImageView: Class for managing Vulkan image views
	// - Creates and manages a view into an image with specific mipmap levels and array layers
	// - Provides access to the underlying image data and view properties
	class ImageView
	{
	public:
		// GetHandle: Returns the native Vulkan image view handle
		vk::ImageView GetHandle() const
		{
			return imageView.get();
		}

		// GetImageData: Returns a pointer to the underlying image data
		lz::ImageData* GetImageData()
		{
			return imageData;
		}

		// GetImageData: Const version of GetImageData
		const lz::ImageData* GetImageData() const
		{
			return imageData;
		}

		// Get methods for view properties
		uint32_t GetBaseMipLevel() { return baseMipLevel; }
		uint32_t GetMipLevelsCount() { return mipLevelsCount; }
		uint32_t GetBaseArrayLayer() { return baseArrayLayer; }
		uint32_t GetArrayLayersCount() { return arrayLayersCount; }

		// Constructor: Creates an image view for standard images (1D, 2D, 3D)
		// Parameters:
		// - logicalDevice: Logical device for creating the image view
		// - imageData: Underlying image data
		// - baseMipLevel: First mipmap level to include in the view
		// - mipLevelsCount: Number of mipmap levels to include in the view
		// - baseArrayLayer: First array layer to include in the view
		// - arrayLayersCount: Number of array layers to include in the view
		ImageView(vk::Device logicalDevice, lz::ImageData* imageData, uint32_t baseMipLevel, uint32_t mipLevelsCount,
		          uint32_t baseArrayLayer, uint32_t arrayLayersCount)
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

		// Constructor: Creates an image view for cubemap images
		// Parameters:
		// - logicalDevice: Logical device for creating the image view
		// - cubemapImageData: Underlying cubemap image data (must have 6 array layers)
		// - baseMipLevel: First mipmap level to include in the view
		// - mipLevelsCount: Number of mipmap levels to include in the view
		ImageView(vk::Device logicalDevice, lz::ImageData* cubemapImageData, uint32_t baseMipLevel,
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

	private:
		vk::UniqueImageView imageView;  // Native Vulkan image view handle
		lz::ImageData* imageData;       // Underlying image data

		uint32_t baseMipLevel;          // First mipmap level in the view
		uint32_t mipLevelsCount;        // Number of mipmap levels in the view

		uint32_t baseArrayLayer;        // First array layer in the view
		uint32_t arrayLayersCount;      // Number of array layers in the view

		friend class lz::Swapchain;
		friend class lz::RenderTarget;
	};
}
