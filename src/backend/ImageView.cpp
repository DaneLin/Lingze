#include "ImageView.h"

#include "Image.h"

namespace lz
{
vk::ImageView ImageView::get_handle() const
{
	return image_view_.get();
}

lz::ImageData *ImageView::get_image_data()
{
	return image_data_;
}

const lz::ImageData *ImageView::get_image_data() const
{
	return image_data_;
}

uint32_t ImageView::get_base_mip_level() const
{
	return base_mip_level_;
}

uint32_t ImageView::get_mip_levels_count() const
{
	return mip_levels_count_;
}

uint32_t ImageView::get_base_array_layer() const
{
	return base_array_layer_;
}

uint32_t ImageView::get_array_layers_count() const
{
	return array_layers_count_;
}

ImageView::ImageView(const vk::Device logical_device, lz::ImageData *image_data, const uint32_t base_mip_level,
                     const uint32_t mip_levels_count, const uint32_t base_array_layer, const uint32_t array_layers_count)
{
	this->image_data_         = image_data;
	this->base_mip_level_     = base_mip_level;
	this->mip_levels_count_   = mip_levels_count;
	this->base_array_layer_   = base_array_layer;
	this->array_layers_count_ = array_layers_count;

	const vk::Format format = image_data->get_format();

	// Configure subresource range
	const auto subresource_range = vk::ImageSubresourceRange()
	                                   .setAspectMask(image_data->get_aspect_flags())
	                                   .setBaseMipLevel(base_mip_level)
	                                   .setLevelCount(mip_levels_count)
	                                   .setBaseArrayLayer(base_array_layer)
	                                   .setLayerCount(array_layers_count);

	// Determine view type based on image type
	vk::ImageViewType view_type;
	if (image_data->get_type() == vk::ImageType::e1D)
		view_type = vk::ImageViewType::e1D;
	if (image_data->get_type() == vk::ImageType::e2D)
		view_type = vk::ImageViewType::e2D;
	if (image_data->get_type() == vk::ImageType::e3D)
		view_type = vk::ImageViewType::e3D;

	// Create the image view
	const auto image_view_create_info = vk::ImageViewCreateInfo()
	                                        .setImage(image_data->get_handle())
	                                        .setViewType(view_type)
	                                        .setFormat(format)
	                                        .setSubresourceRange(subresource_range);
	image_view_ = logical_device.createImageViewUnique(image_view_create_info);
}

ImageView::ImageView(const vk::Device logical_device, lz::ImageData *cubemap_image_data, const uint32_t base_mip_level,
                     const uint32_t mip_levels_count) :
    image_data_(cubemap_image_data), base_mip_level_(base_mip_level), mip_levels_count_(mip_levels_count), base_array_layer_(0), array_layers_count_(0)

{
	const vk::Format     format = image_data_->get_format();
	vk::ImageAspectFlags aspect_flags;

	// Configure subresource range for cubemap (always 6 layers)
	const auto subresource_range = vk::ImageSubresourceRange()
	                                   .setAspectMask(image_data_->get_aspect_flags())
	                                   .setBaseMipLevel(base_mip_level)
	                                   .setLevelCount(mip_levels_count)
	                                   .setBaseArrayLayer(base_array_layer_)
	                                   .setLayerCount(array_layers_count_);

	// Cubemap view type
	constexpr vk::ImageViewType view_type = vk::ImageViewType::eCube;

	// Validation checks for cubemap
	assert(image_data_->get_type() == vk::ImageType::e2D);
	assert(image_data_->get_array_layers_count() == 6);

	// Create the cubemap image view
	const auto image_view_create_info = vk::ImageViewCreateInfo()
	                                        .setImage(image_data_->get_handle())
	                                        .setViewType(view_type)
	                                        .setFormat(format)
	                                        .setSubresourceRange(subresource_range);
	image_view_ = logical_device.createImageViewUnique(image_view_create_info);
}
}        // namespace lz
