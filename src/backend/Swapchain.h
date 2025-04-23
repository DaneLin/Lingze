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
		vk::Format GetFormat();

		// GetSize: Returns the size of the swapchain images
		vk::Extent2D GetSize();

		// GetImageViews: Returns all image views in the swapchain
		std::vector<ImageView*> GetImageViews();

		// AcquireNextImage: Acquires the next available image from the swapchain
		// Parameters:
		//   - semaphore: Semaphore to signal when image acquisition is complete
		// Returns: Result value containing the index of the acquired image
		vk::ResultValue<uint32_t> AcquireNextImage(vk::Semaphore semaphore);

		// GetHandle: Returns the native Vulkan swapchain handle
		vk::SwapchainKHR GetHandle();

		// Recreate: Rebuilds the swapchain, typically called when window size changes
		// Parameters:
		//   - newSize: New dimensions for the swapchain
		// Returns: True if recreation was successful, false otherwise
		bool Recreate(vk::Extent2D newSize);

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
		Swapchain(vk::Instance instance, vk::PhysicalDevice physicalDevice, vk::Device logicalDevice,
		          WindowDesc windowDesc, uint32_t imagesCount, QueueFamilyIndices queueFamilyIndices,
		          vk::PresentModeKHR preferredMode);

		// SurfaceDetails: Structure to hold surface capabilities, formats and present modes
		struct SurfaceDetails
		{
			vk::SurfaceCapabilitiesKHR capabilities;
			std::vector<vk::SurfaceFormatKHR> formats;
			std::vector<vk::PresentModeKHR> presentModes;
		};

		// GetSurfaceDetails: Queries and returns surface details from the physical device
		// Parameters:
		//   - physicalDevice: Physical device to query capabilities from
		//   - surface: Surface to query details for
		// Returns: Surface details including capabilities, formats and present modes
		static SurfaceDetails GetSurfaceDetails(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface);

		static vk::SurfaceFormatKHR FindSwapchainSurfaceFormat(
			const std::vector<vk::SurfaceFormatKHR>& availableFormats);

		static vk::PresentModeKHR FindSwapchainPresentMode(const std::vector<vk::PresentModeKHR> availablePresentModes,
		                                                   vk::PresentModeKHR preferredMode);

		static vk::Extent2D FindSwapchainExtent(const vk::SurfaceCapabilitiesKHR& capabilities,
		                                        vk::Extent2D windowSize);

		SurfaceDetails surfaceDetails;
		vk::PhysicalDevice physicalDevice;
		vk::Device logicalDevice;
		vk::SurfaceFormatKHR surfaceFormat;
		vk::PresentModeKHR presentMode;
		vk::Extent2D extent;
		QueueFamilyIndices queueFamilyIndices;
		uint32_t desiredImageCount;

		struct Image
		{
			std::unique_ptr<lz::ImageData> imageData;
			std::unique_ptr<lz::ImageView> imageView;
		};

		std::vector<Image> images;

		vk::UniqueSurfaceKHR surface;
		vk::UniqueSwapchainKHR swapchain;

		friend class Core;
	};
}
