#include "Image.h"

#include "Buffer.h"

namespace lz
{
bool ImageSubresourceRange::contains(const ImageSubresourceRange &other) const
{
	return base_mip_level <= other.base_mip_level && base_array_layer <= other.base_array_layer && mips_count >= other.mips_count && array_layers_count >= other.array_layers_count;
}

bool ImageSubresourceRange::operator<(const ImageSubresourceRange &other) const
{
	return std::tie(base_mip_level, mips_count, base_array_layer, array_layers_count) < std::tie(other.base_mip_level, other.mips_count, other.base_array_layer, other.array_layers_count);
}

vk::Image ImageData::get_handle() const
{
	return image_handle_;
}

vk::Format ImageData::get_format() const
{
	return format_;
}

vk::ImageType ImageData::get_type() const
{
	return image_type_;
}

glm::uvec3 ImageData::get_mip_size(const uint32_t mip_level) const
{
	return mip_infos_[mip_level].size;
}

vk::ImageAspectFlags ImageData::get_aspect_flags() const
{
	return aspect_flags_;
}

uint32_t ImageData::get_array_layers_count() const
{
	return array_layers_count_;
}

uint32_t ImageData::get_mips_count() const
{
	return mips_count_;
}

bool ImageData::operator<(const ImageData &other) const
{
	return std::tie(image_handle_) < std::tie(other.image_handle_);
}

ImageData::ImageData(vk::Image image_handle, vk::ImageType image_type, const glm::uvec3 size, const uint32_t mipsCount,
                     const uint32_t array_layers_count, const vk::Format format, const vk::ImageLayout layout)
{
	this->image_handle_       = image_handle;
	this->format_             = format;
	this->mips_count_         = mipsCount;
	this->array_layers_count_ = array_layers_count;
	this->image_type_         = image_type;

	glm::vec3 curr_size = size;

	for (size_t mip_level = 0; mip_level < mipsCount; ++mip_level)
	{
		MipInfo mip_info;
		mip_info.size = curr_size;
		curr_size.x /= 2;
		if (image_type == vk::ImageType::e2D || image_type == vk::ImageType::e3D)
		{
			curr_size.y /= 2;
		}
		if (image_type == vk::ImageType::e3D)
		{
			curr_size.z /= 2;
		}

		mip_info.layer_infos.resize(array_layers_count);

		for (size_t layer_index = 0; layer_index < array_layers_count; ++layer_index)
		{
			mip_info.layer_infos[layer_index].curr_layout = layout;
		}
		mip_infos_.push_back(mip_info);
	}
	if (lz::is_depth_format(format))
	{
		this->aspect_flags_ = vk::ImageAspectFlagBits::eDepth;
	}
	else
	{
		this->aspect_flags_ = vk::ImageAspectFlagBits::eColor;
	}
}

void ImageData::set_debug_name(const std::string &debug_name)
{
	this->debug_name_ = debug_name;
}

lz::ImageData *Image::get_image_data() const
{
	return image_data_.get();
}

vk::DeviceMemory Image::get_memory()
{
	return image_memory_.get();
}

vk::ImageCreateInfo Image::create_info_2d(const glm::uvec2 size, const uint32_t mips_count, const uint32_t array_layers_count,
                                          const vk::Format format, const vk::ImageUsageFlags usage)
{
	constexpr auto layout     = vk::ImageLayout::eUndefined;
	const auto     image_info = vk::ImageCreateInfo()
	                            .setImageType(vk::ImageType::e2D)
	                            .setExtent(vk::Extent3D(size.x, size.y, 1))
	                            .setMipLevels(mips_count)
	                            .setArrayLayers(array_layers_count)
	                            .setFormat(format)
	                            .setInitialLayout(layout)
	                            .setUsage(usage)
	                            .setSharingMode(vk::SharingMode::eExclusive)
	                            .setSamples(vk::SampleCountFlagBits::e1)
	                            .setFlags(vk::ImageCreateFlags());

	return image_info;
}

vk::ImageCreateInfo Image::create_info_volume(const glm::uvec3 size, const uint32_t mips_count, const uint32_t array_layers_count,
                                              const vk::Format format, const vk::ImageUsageFlags usage)
{
	constexpr auto layout     = vk::ImageLayout::eUndefined;
	const auto     image_info = vk::ImageCreateInfo()
	                            .setImageType(vk::ImageType::e3D)
	                            .setExtent(vk::Extent3D(size.x, size.y, size.z))
	                            .setMipLevels(mips_count)
	                            .setArrayLayers(array_layers_count)
	                            .setFormat(format)
	                            .setInitialLayout(layout)        // images must be created in undefined layout
	                            .setUsage(usage)
	                            .setSharingMode(vk::SharingMode::eExclusive)
	                            .setSamples(vk::SampleCountFlagBits::e1)
	                            .setFlags(vk::ImageCreateFlags())
	                            .setTiling(vk::ImageTiling::eOptimal);
	return image_info;
}

vk::ImageCreateInfo Image::create_info_cube(const glm::uvec2 size, const uint32_t mips_count, const vk::Format format,
                                            const vk::ImageUsageFlags usage)
{
	constexpr auto layout     = vk::ImageLayout::eUndefined;
	const auto     image_info = vk::ImageCreateInfo()
	                            .setImageType(vk::ImageType::e2D)
	                            .setExtent(vk::Extent3D(size.x, size.y, 1))
	                            .setMipLevels(mips_count)
	                            .setArrayLayers(6)        // Cubemap has 6 faces
	                            .setFormat(format)
	                            .setInitialLayout(layout)
	                            .setUsage(usage)
	                            .setSharingMode(vk::SharingMode::eExclusive)
	                            .setSamples(vk::SampleCountFlagBits::e1)
	                            .setFlags(vk::ImageCreateFlagBits::eCubeCompatible);

	return image_info;
}

Image::Image(const vk::PhysicalDevice physical_device, const vk::Device logical_device, const vk::ImageCreateInfo &image_info,
             const vk::MemoryPropertyFlags mem_flags)
{
	// Create the image resource
	image_handle_   = logical_device.createImageUnique(image_info);
	glm::uvec3 size = {image_info.extent.width, image_info.extent.height, image_info.extent.depth};

	// Create image data object to track image properties and layout
	image_data_.reset(new lz::ImageData(image_handle_.get(), image_info.imageType, size, image_info.mipLevels,
	                                    image_info.arrayLayers, image_info.format, image_info.initialLayout));

	// Get memory requirements for the image
	vk::MemoryRequirements imageMemRequirements = logical_device.getImageMemoryRequirements(image_handle_.get());

	// Allocate memory for the image
	auto allocInfo = vk::MemoryAllocateInfo()
	                     .setAllocationSize(imageMemRequirements.size)
	                     .setMemoryTypeIndex(
	                         lz::find_memory_type_index(physical_device, imageMemRequirements.memoryTypeBits,
	                                                    mem_flags));

	image_memory_ = logical_device.allocateMemoryUnique(allocInfo);

	// Bind the image to the allocated memory
	logical_device.bindImageMemory(image_handle_.get(), image_memory_.get(), 0);
}
}        // namespace lz
