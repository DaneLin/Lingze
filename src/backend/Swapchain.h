#pragma once
#include <iostream>

#include "Image.h"
#include "ImageView.h"
#include "LingzeVK.h"
#include "QueueIndices.h"
#include "Surface.h"

namespace lz
{
	// Swapchain: Manages the Vulkan swapchain for image presentation to the window
	class Swapchain
	{
	public:
		// GetFormat: Returns the image format of the swapchain
		vk::Format get_format() const;

		// GetSize: Returns the size of the swapchain images
		vk::Extent2D get_size() const;

		// GetImageViews: Returns all image views in the swapchain
		std::vector<ImageView*> get_image_views();

		// AcquireNextImage: Acquires the next available image from the swapchain
		// Parameters:
		//   - semaphore: Semaphore to signal when image acquisition is complete
		// Returns: Result value containing the index of the acquired image
		vk::ResultValue<uint32_t> acquire_next_image(vk::Semaphore semaphore);

		// GetHandle: Returns the native Vulkan swapchain handle
		vk::SwapchainKHR get_handle();

		// Recreate: Rebuilds the swapchain, typically called when window size changes
		// Parameters:
		//   - newSize: New dimensions for the swapchain
		// Returns: True if recreation was successful, false otherwise
		bool recreate(vk::Extent2D new_size);

	private:
		// Private constructor - called by Core::CreateSwapchain
		// Parameters:
		//   - instance: Vulkan instance
		//   - physicalDevice: Physical device for rendering
		//   - logicalDevice: Logical device for rendering
		//   - windowDesc: Window descriptor for surface creation
		//   - imagesCount: Desired number of images in the swapchain
		//   - queueFamilyIndices: Queue family indices for graphics and presentation
		//   - preferredMode: Preferred presentation mode
		Swapchain(vk::Instance instance, vk::PhysicalDevice physical_device, vk::Device logical_device,
		          WindowDesc window_desc, uint32_t images_count, QueueFamilyIndices queue_family_indices,
		          vk::PresentModeKHR preferred_mode);

		// SurfaceDetails: Structure to hold surface capabilities, formats and present modes
		struct SurfaceDetails
		{
			vk::SurfaceCapabilitiesKHR capabilities;
			std::vector<vk::SurfaceFormatKHR> formats;
			std::vector<vk::PresentModeKHR> present_modes;
		};

		// GetSurfaceDetails: Queries and returns surface details from the physical device
		// Parameters:
		//   - physicalDevice: Physical device to query capabilities from
		//   - surface: Surface to query details for
		// Returns: Surface details including capabilities, formats and present modes
		static SurfaceDetails get_surface_details(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface);

		static vk::SurfaceFormatKHR find_swapchain_surface_format(
			const std::vector<vk::SurfaceFormatKHR>& availableFormats);

		static vk::PresentModeKHR find_swapchain_present_mode(const std::vector<vk::PresentModeKHR> available_present_modes,
		                                                   vk::PresentModeKHR preferred_mode);

		static vk::Extent2D find_swapchain_extent(const vk::SurfaceCapabilitiesKHR& capabilities,
		                                        vk::Extent2D window_size);

		SurfaceDetails surface_details_;
		vk::PhysicalDevice physical_device_;
		vk::Device logical_device_;
		vk::SurfaceFormatKHR surface_format_;
		vk::PresentModeKHR present_mode_;
		vk::Extent2D extent_;
		QueueFamilyIndices queue_family_indices_;
		uint32_t desired_image_count_;

		struct Image
		{
			std::unique_ptr<lz::ImageData> image_data;
			std::unique_ptr<lz::ImageView> image_view;
		};

		std::vector<Image> images_;

		vk::UniqueSurfaceKHR surface_;
		vk::UniqueSwapchainKHR swapchain_;

		friend class Core;
	};
}
