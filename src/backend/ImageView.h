#pragma once

#include "Image.h"
#include "LingzeVK.h"

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
		vk::ImageView GetHandle() const;

		// GetImageData: Returns a pointer to the underlying image data
		lz::ImageData* GetImageData();

		// GetImageData: Const version of GetImageData
		const lz::ImageData* GetImageData() const;

		// Get methods for view properties
		uint32_t GetBaseMipLevel();
		uint32_t GetMipLevelsCount();
		uint32_t GetBaseArrayLayer();
		uint32_t GetArrayLayersCount();

		// Constructor: Creates an image view for standard images (1D, 2D, 3D)
		// Parameters:
		// - logicalDevice: Logical device for creating the image view
		// - imageData: Underlying image data
		// - baseMipLevel: First mipmap level to include in the view
		// - mipLevelsCount: Number of mipmap levels to include in the view
		// - baseArrayLayer: First array layer to include in the view
		// - arrayLayersCount: Number of array layers to include in the view
		ImageView(vk::Device logicalDevice, lz::ImageData* imageData, uint32_t baseMipLevel, uint32_t mipLevelsCount,
		          uint32_t baseArrayLayer, uint32_t arrayLayersCount);

		// Constructor: Creates an image view for cubemap images
		// Parameters:
		// - logicalDevice: Logical device for creating the image view
		// - cubemapImageData: Underlying cubemap image data (must have 6 array layers)
		// - baseMipLevel: First mipmap level to include in the view
		// - mipLevelsCount: Number of mipmap levels to include in the view
		ImageView(vk::Device logicalDevice, lz::ImageData* cubemapImageData, uint32_t baseMipLevel,
		          uint32_t mipLevelsCount);

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
