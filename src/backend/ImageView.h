#pragma once

#include "Image.h"
#include "Config.h"

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
	vk::ImageView get_handle() const;

	// GetImageData: Returns a pointer to the underlying image data
	lz::ImageData *get_image_data();

	// GetImageData: Const version of GetImageData
	const lz::ImageData *get_image_data() const;

	// Get methods for view properties
	uint32_t get_base_mip_level() const;
	uint32_t get_mip_levels_count() const;
	uint32_t get_base_array_layer() const;
	uint32_t get_array_layers_count() const;

	// Constructor: Creates an image view for standard images (1D, 2D, 3D)
	// Parameters:
	// - logicalDevice: Logical device for creating the image view
	// - imageData: Underlying image data
	// - baseMipLevel: First mipmap level to include in the view
	// - mipLevelsCount: Number of mipmap levels to include in the view
	// - baseArrayLayer: First array layer to include in the view
	// - arrayLayersCount: Number of array layers to include in the view
	ImageView(vk::Device logical_device, lz::ImageData *image_data,
	          uint32_t base_mip_level, uint32_t mip_levels_count,
	          uint32_t base_array_layer, uint32_t array_layers_count);

	// Constructor: Creates an image view for cubemap images
	// Parameters:
	// - logicalDevice: Logical device for creating the image view
	// - cubemapImageData: Underlying cubemap image data (must have 6 array layers)
	// - baseMipLevel: First mipmap level to include in the view
	// - mipLevelsCount: Number of mipmap levels to include in the view
	ImageView(vk::Device logical_device, lz::ImageData *cubemap_image_data, uint32_t base_mip_level, uint32_t mip_levels_count);

  private:
	vk::UniqueImageView image_view_;        // Native Vulkan image view handle
	lz::ImageData      *image_data_;        // Underlying image data

	uint32_t base_mip_level_;          // First mipmap level in the view
	uint32_t mip_levels_count_;        // Number of mipmap levels in the view

	uint32_t base_array_layer_;          // First array layer in the view
	uint32_t array_layers_count_;        // Number of array layers in the view

	friend class lz::Swapchain;
	friend class lz::RenderTarget;
};
}        // namespace lz
