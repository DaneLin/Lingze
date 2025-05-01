#include "Swapchain.h"

#include "Image.h"
#include "ImageView.h"
#include "QueueIndices.h"
#include "Surface.h"

namespace lz
{
vk::Format Swapchain::get_format() const
{
	return this->surface_format_.format;
}

vk::Extent2D Swapchain::get_size() const
{
	return extent_;
}

std::vector<ImageView *> Swapchain::get_image_views()
{
	std::vector<ImageView *> res_image_views;
	for (auto &image : this->images_)
		res_image_views.push_back(image.image_view.get());
	return res_image_views;
}

vk::ResultValue<uint32_t> Swapchain::acquire_next_image(vk::Semaphore semaphore)
{
	return logical_device_.acquireNextImageKHR(swapchain_.get(), std::numeric_limits<uint64_t>::max(), semaphore,
	                                           nullptr);
}

vk::SwapchainKHR Swapchain::get_handle()
{
	return swapchain_.get();
}

bool Swapchain::recreate(vk::Extent2D new_size)
{
	// Query surface capabilities again
	this->surface_details_ = get_surface_details(physical_device_, surface_.get());

	// Recalculate swapchain extent using the new window size
	this->extent_ = find_swapchain_extent(surface_details_.capabilities, new_size);

	// Clear old images and image views
	this->images_.clear();

	// Create new swapchain with the same image count as before
	uint32_t image_count = std::max(surface_details_.capabilities.minImageCount, desired_image_count_);
	if (surface_details_.capabilities.maxImageCount > 0 && image_count > surface_details_.capabilities.maxImageCount)
		image_count = surface_details_.capabilities.maxImageCount;

	auto swapchain_create_info = vk::SwapchainCreateInfoKHR()
	                                 .setSurface(surface_.get())
	                                 .setMinImageCount(image_count)
	                                 .setImageFormat(surface_format_.format)
	                                 .setImageColorSpace(surface_format_.colorSpace)
	                                 .setImageExtent(extent_)
	                                 .setImageArrayLayers(1)
	                                 .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
	                                 .setPreTransform(surface_details_.capabilities.currentTransform)
	                                 .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
	                                 .setPresentMode(present_mode_)
	                                 .setClipped(true)
	                                 .setOldSwapchain(swapchain_.get());        // Use old swapchain as reference

	const uint32_t family_indices[] = {
	    queue_family_indices_.graphics_family_index, queue_family_indices_.present_family_index};

	// Configure image sharing mode based on queue family indices
	if (queue_family_indices_.graphics_family_index != queue_family_indices_.present_family_index)
	{
		swapchain_create_info
		    .setImageSharingMode(vk::SharingMode::eConcurrent)
		    .setQueueFamilyIndexCount(2)
		    .setPQueueFamilyIndices(family_indices);
	}
	else
	{
		swapchain_create_info
		    .setImageSharingMode(vk::SharingMode::eExclusive);
	}

	// Create the new swapchain
	vk::UniqueSwapchainKHR new_swapchain;
	try
	{
		new_swapchain = logical_device_.createSwapchainKHRUnique(swapchain_create_info);
	}
	catch (vk::SystemError &err)
	{
		std::cerr << "Failed to recreate swapchain: " << err.what() << std::endl;
		return false;
	}

	// Replace and clean up old swapchain
	swapchain_ = std::move(new_swapchain);

	// Get new swapchain images and create image views
	const std::vector<vk::Image> swapchain_images = logical_device_.getSwapchainImagesKHR(swapchain_.get());

	for (size_t image_index = 0; image_index < swapchain_images.size(); image_index++)
	{
		Image newbie;
		newbie.image_data = std::unique_ptr<ImageData>(new ImageData(
		    swapchain_images[image_index], vk::ImageType::e2D, glm::vec3(extent_.width, extent_.height, 1), 1, 1,
		    surface_format_.format, vk::ImageLayout::eUndefined));
		newbie.image_view = std::unique_ptr<ImageView>(
		    new ImageView(logical_device_, newbie.image_data.get(), 0, 1, 0, 1));

		this->images_.emplace_back(std::move(newbie));
	}

	return true;
}

Swapchain::Swapchain(vk::Instance instance, vk::PhysicalDevice physical_device, vk::Device logical_device,
                     WindowDesc window_desc, uint32_t images_count, QueueFamilyIndices queue_family_indices,
                     vk::PresentModeKHR preferred_mode) :
    logical_device_(logical_device), physical_device_(physical_device), queue_family_indices_(queue_family_indices), desired_image_count_(images_count), present_mode_(preferred_mode)
{
	// Create Win32 surface for the window
	this->surface_ = lz::create_win32_surface(instance, window_desc);
	if (queue_family_indices.present_family_index == uint32_t(-1) || !physical_device.getSurfaceSupportKHR(
	                                                                     queue_family_indices.present_family_index, surface_.get()))
		throw std::runtime_error("Window surface is incompatible with device");

	// Get surface details (capabilities, formats, present modes)
	this->surface_details_ = get_surface_details(physical_device, surface_.get());

	// Select appropriate format, present mode and extent
	this->surface_format_ = find_swapchain_surface_format(surface_details_.formats);
	this->present_mode_   = find_swapchain_present_mode(surface_details_.present_modes, preferred_mode);
	this->extent_         = find_swapchain_extent(surface_details_.capabilities, vk::Extent2D(100, 100));

	// Determine the number of images in the swapchain
	uint32_t image_count = std::max(surface_details_.capabilities.minImageCount, images_count);
	if (surface_details_.capabilities.maxImageCount > 0 && image_count > surface_details_.capabilities.maxImageCount)
		image_count = surface_details_.capabilities.maxImageCount;

	// Configure swapchain creation parameters
	auto swapchain_create_info = vk::SwapchainCreateInfoKHR()
	                                 .setSurface(surface_.get())
	                                 .setMinImageCount(image_count)
	                                 .setImageFormat(surface_format_.format)
	                                 .setImageColorSpace(surface_format_.colorSpace)
	                                 .setImageExtent(extent_)
	                                 .setImageArrayLayers(1)
	                                 .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
	                                 .setPreTransform(surface_details_.capabilities.currentTransform)
	                                 .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
	                                 .setPresentMode(present_mode_)
	                                 .setClipped(true)
	                                 .setOldSwapchain(nullptr);

	const uint32_t family_indices[] = {
	    queue_family_indices.graphics_family_index, queue_family_indices.present_family_index};

	// Configure image sharing mode based on queue family indices
	if (queue_family_indices.graphics_family_index != queue_family_indices.present_family_index)
	{
		swapchain_create_info
		    .setImageSharingMode(vk::SharingMode::eConcurrent)
		    .setQueueFamilyIndexCount(2)
		    .setPQueueFamilyIndices(family_indices);
	}
	else
	{
		swapchain_create_info
		    .setImageSharingMode(vk::SharingMode::eExclusive);
	}

	// Create the swapchain
	this->swapchain_ = logical_device.createSwapchainKHRUnique(swapchain_create_info);

	// Get the swapchain images
	const std::vector<vk::Image> swapchain_images = logical_device.getSwapchainImagesKHR(swapchain_.get());

	// Create image views for all swapchain images
	this->images_.clear();
	for (size_t image_index = 0; image_index < swapchain_images.size(); image_index++)
	{
		Image newbie;
		newbie.image_data = std::unique_ptr<ImageData>(new ImageData(
		    swapchain_images[image_index], vk::ImageType::e2D, glm::vec3(extent_.width, extent_.height, 1), 1, 1,
		    surface_format_.format, vk::ImageLayout::eUndefined));
		newbie.image_view = std::unique_ptr<ImageView>(
		    new ImageView(logical_device, newbie.image_data.get(), 0, 1, 0, 1));

		this->images_.emplace_back(std::move(newbie));
	}
}

Swapchain::SurfaceDetails Swapchain::get_surface_details(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface)
{
	SurfaceDetails surface_details;
	surface_details.capabilities  = physical_device.getSurfaceCapabilitiesKHR(surface);
	surface_details.formats       = physical_device.getSurfaceFormatsKHR(surface);
	surface_details.present_modes = physical_device.getSurfacePresentModesKHR(surface);

	return surface_details;
}

vk::SurfaceFormatKHR Swapchain::find_swapchain_surface_format(
    const std::vector<vk::SurfaceFormatKHR> &availableFormats)
{
	constexpr vk::SurfaceFormatKHR best_format = {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear};

	if (availableFormats.size() == 1 && availableFormats[0].format == vk::Format::eUndefined)
	{
		return best_format;
	}

	for (const auto &available_format : availableFormats)
	{
		if (available_format.format == best_format.format && available_format.colorSpace == best_format.colorSpace)
			return available_format;
	}
	throw std::runtime_error("No suitable format found");
	return best_format;
}

vk::PresentModeKHR Swapchain::find_swapchain_present_mode(
    const std::vector<vk::PresentModeKHR> available_present_modes,
    vk::PresentModeKHR                    preferred_mode)
{
	for (const auto &available_present_mode : available_present_modes)
	{
		if (available_present_mode == preferred_mode)
			return available_present_mode;
	}

	return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Swapchain::find_swapchain_extent(const vk::SurfaceCapabilitiesKHR &capabilities,
                                              vk::Extent2D                      window_size)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		vk::Extent2D actual_extent = window_size;

		actual_extent.width  = std::max(capabilities.minImageExtent.width,
		                                std::min(capabilities.maxImageExtent.width, actual_extent.width));
		actual_extent.height = std::max(capabilities.minImageExtent.height,
		                                std::min(capabilities.maxImageExtent.height, actual_extent.height));

		return actual_extent;
	}
}
}        // namespace lz
