#pragma once

#include "LingzeVK.h"

namespace lz
{
	// IsDepthFormat: Checks if a format is a depth format
	// Parameters:
	// - format: Vulkan format to check
	// Returns: True if the format is a depth format, false otherwise
	static bool is_depth_format(const vk::Format format)
	{
		return (format >= vk::Format::eD16Unorm && format < vk::Format::eD32SfloatS8Uint);
	}

	// GetGeneralUsageFlags: Determines appropriate usage flags for an image based on its format
	// Parameters:
	// - format: Vulkan format of the image
	// Returns: Appropriate image usage flags for the given format
	static vk::ImageUsageFlags get_general_usage_flags(const vk::Format format)
	{
		vk::ImageUsageFlags usage_flags = vk::ImageUsageFlagBits::eSampled;
		if (is_depth_format(format))
		{
			usage_flags |= vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled;
		}
		else
		{
			usage_flags |= vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst |
				vk::ImageUsageFlagBits::eSampled;
		}
		return usage_flags;
	}

	// Common image usage flag combinations
	static constexpr vk::ImageUsageFlags color_image_usage = vk::ImageUsageFlagBits::eColorAttachment |
		vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
	static constexpr vk::ImageUsageFlags depth_image_usage = vk::ImageUsageFlagBits::eDepthStencilAttachment |
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
		bool contains(const ImageSubresourceRange& other) const;

		// Comparison operator for container sorting
		bool operator <(const ImageSubresourceRange& other) const;

		uint32_t base_mip_level; // First mipmap level in the range
		uint32_t mips_count; // Number of mipmap levels in the range
		uint32_t base_array_layer; // First array layer in the range
		uint32_t array_layers_count; // Number of array layers in the range
	};


	// ImageData: Class that holds information about a Vulkan image without owning the resource
	// - Contains image metadata and layout tracking for each mip level and array layer
	class ImageData
	{
	public:
		// GetHandle: Returns the native Vulkan image handle
		vk::Image get_handle() const;

		// GetFormat: Returns the format of the image
		vk::Format get_format() const;

		// GetType: Returns the type of the image (1D, 2D, or 3D)
		vk::ImageType get_type() const;

		// GetMipSize: Returns the size of a specific mipmap level
		// Parameters:
		// - mipLevel: Mipmap level to query
		// Returns: Size of the specified mipmap level
		glm::uvec3 get_mip_size(uint32_t mip_level) const;

		// GetAspectFlags: Returns the aspect flags of the image (color or depth)
		vk::ImageAspectFlags get_aspect_flags() const;

		uint32_t get_array_layers_count() const;

		uint32_t get_mips_count() const;

		bool operator <(const ImageData& other) const;

	private:
		ImageData(vk::Image image_handle, vk::ImageType image_type, glm::uvec3 size, uint32_t mipsCount,
		          uint32_t array_layers_count, vk::Format format, vk::ImageLayout layout);

		void set_debug_name(const std::string& debug_name);

		struct SubImageInfo
		{
			vk::ImageLayout curr_layout;
		};

		struct MipInfo
		{
			std::vector<SubImageInfo> layer_infos;
			glm::uvec3 size;
		};

		std::vector<MipInfo> mip_infos_;

		vk::ImageAspectFlags aspect_flags_;
		vk::Image image_handle_;
		vk::Format format_;
		vk::ImageType image_type_;
		uint32_t mips_count_;
		uint32_t array_layers_count_;
		std::string debug_name_;

		friend class lz::Image;
		friend class lz::Swapchain;
		friend class Core;
	};


	class Image
	{
	public:
		lz::ImageData* get_image_data() const;

		vk::DeviceMemory get_memory();

		static vk::ImageCreateInfo create_info_2d(glm::uvec2 size, uint32_t mips_count, uint32_t array_layers_count,
		                                        vk::Format format, vk::ImageUsageFlags usage);

		// CreateInfoVolume: Creates an image create info structure for 3D volume images
		// Parameters:
		// - size: Width, height, and depth of the volume
		// - mipsCount: Number of mipmap levels
		// - arrayLayersCount: Number of array layers
		// - format: Format of the image
		// - usage: Usage flags for the image
		// Returns: Configured image create info structure for a 3D volume image
		static vk::ImageCreateInfo create_info_volume(glm::uvec3 size, uint32_t mips_count, uint32_t array_layers_count,
		                                            vk::Format format, vk::ImageUsageFlags usage);

		// CreateInfoCube: Creates an image create info structure for cubemap images
		// Parameters:
		// - size: Width and height of each face of the cubemap
		// - mipsCount: Number of mipmap levels
		// - format: Format of the image
		// - usage: Usage flags for the image
		// Returns: Configured image create info structure for a cubemap
		static vk::ImageCreateInfo create_info_cube(glm::uvec2 size, uint32_t mips_count, vk::Format format,
		                                          vk::ImageUsageFlags usage);

		// Constructor: Creates a new image with specified properties
		// Parameters:
		// - physicalDevice: Physical device for memory allocation
		// - logicalDevice: Logical device for image operations
		// - imageInfo: Image creation parameters
		// - memFlags: Memory property flags for the image allocation
		Image(vk::PhysicalDevice physical_device, vk::Device logical_device, const vk::ImageCreateInfo& image_info,
		      vk::MemoryPropertyFlags mem_flags = vk::MemoryPropertyFlagBits::eDeviceLocal);

	private:
		vk::UniqueImage image_handle_; // Native Vulkan image handle
		std::unique_ptr<lz::ImageData> image_data_; // Image metadata and layout tracking
		vk::UniqueDeviceMemory image_memory_; // Device memory allocation for this image
	};
}
