#include "Swapchain.h"

#include "Image.h"
#include "ImageView.h"
#include "QueueIndices.h"
#include "Surface.h"

namespace lz
{
	vk::Format Swapchain::GetFormat()
	{
		return this->surfaceFormat.format;
	}

	vk::Extent2D Swapchain::GetSize()
	{
		return extent;
	}

	std::vector<ImageView*> Swapchain::GetImageViews()
	{
		std::vector<ImageView*> resImageViews;
		for (auto& image : this->images)
			resImageViews.push_back(image.imageView.get());
		return resImageViews;
	}

	vk::ResultValue<uint32_t> Swapchain::AcquireNextImage(vk::Semaphore semaphore)
	{
		return logicalDevice.acquireNextImageKHR(swapchain.get(), std::numeric_limits<uint64_t>::max(), semaphore,
		                                         nullptr);
	}

	vk::SwapchainKHR Swapchain::GetHandle()
	{
		return swapchain.get();
	}

	bool Swapchain::Recreate(vk::Extent2D newSize)
	{
		// Query surface capabilities again
		this->surfaceDetails = GetSurfaceDetails(physicalDevice, surface.get());

		// Recalculate swapchain extent using the new window size
		this->extent = FindSwapchainExtent(surfaceDetails.capabilities, newSize);

		// Clear old images and image views
		this->images.clear();

		// Create new swapchain with the same image count as before
		uint32_t imageCount = std::max(surfaceDetails.capabilities.minImageCount, desiredImageCount);
		if (surfaceDetails.capabilities.maxImageCount > 0 && imageCount > surfaceDetails.capabilities.maxImageCount)
			imageCount = surfaceDetails.capabilities.maxImageCount;

		auto swapchainCreateInfo = vk::SwapchainCreateInfoKHR()
		                           .setSurface(surface.get())
		                           .setMinImageCount(imageCount)
		                           .setImageFormat(surfaceFormat.format)
		                           .setImageColorSpace(surfaceFormat.colorSpace)
		                           .setImageExtent(extent)
		                           .setImageArrayLayers(1)
		                           .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
		                           .setPreTransform(surfaceDetails.capabilities.currentTransform)
		                           .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
		                           .setPresentMode(presentMode)
		                           .setClipped(true)
		                           .setOldSwapchain(swapchain.get()); // Use old swapchain as reference

		uint32_t familyIndices[] = {queueFamilyIndices.graphicsFamilyIndex, queueFamilyIndices.presentFamilyIndex};

		// Configure image sharing mode based on queue family indices
		if (queueFamilyIndices.graphicsFamilyIndex != queueFamilyIndices.presentFamilyIndex)
		{
			swapchainCreateInfo
				.setImageSharingMode(vk::SharingMode::eConcurrent)
				.setQueueFamilyIndexCount(2)
				.setPQueueFamilyIndices(familyIndices);
		}
		else
		{
			swapchainCreateInfo
				.setImageSharingMode(vk::SharingMode::eExclusive);
		}

		// Create the new swapchain
		vk::UniqueSwapchainKHR newSwapchain;
		try
		{
			newSwapchain = logicalDevice.createSwapchainKHRUnique(swapchainCreateInfo);
		}
		catch (vk::SystemError& err)
		{
			std::cerr << "Failed to recreate swapchain: " << err.what() << std::endl;
			return false;
		}

		// Replace and clean up old swapchain
		swapchain = std::move(newSwapchain);

		// Get new swapchain images and create image views
		std::vector<vk::Image> swapchainImages = logicalDevice.getSwapchainImagesKHR(swapchain.get());

		for (size_t imageIndex = 0; imageIndex < swapchainImages.size(); imageIndex++)
		{
			Image newbie;
			newbie.imageData = std::unique_ptr<ImageData>(new ImageData(
				swapchainImages[imageIndex], vk::ImageType::e2D, glm::vec3(extent.width, extent.height, 1), 1, 1,
				surfaceFormat.format, vk::ImageLayout::eUndefined));
			newbie.imageView = std::unique_ptr<ImageView>(
				new ImageView(logicalDevice, newbie.imageData.get(), 0, 1, 0, 1));

			this->images.emplace_back(std::move(newbie));
		}

		return true;
	}

	Swapchain::Swapchain(vk::Instance instance, vk::PhysicalDevice physicalDevice, vk::Device logicalDevice,
	                     WindowDesc windowDesc, uint32_t imagesCount, QueueFamilyIndices queueFamilyIndices,
	                     vk::PresentModeKHR preferredMode)
	{
		this->logicalDevice = logicalDevice;
		this->physicalDevice = physicalDevice;
		this->queueFamilyIndices = queueFamilyIndices;
		this->desiredImageCount = imagesCount;
		this->presentMode = preferredMode;

		// Create Win32 surface for the window
		this->surface = lz::CreateWin32Surface(instance, windowDesc);
		if (queueFamilyIndices.presentFamilyIndex == uint32_t(-1) || !physicalDevice.getSurfaceSupportKHR(
			queueFamilyIndices.presentFamilyIndex, surface.get()))
			throw std::runtime_error("Window surface is incompatible with device");

		// Get surface details (capabilities, formats, present modes)
		this->surfaceDetails = GetSurfaceDetails(physicalDevice, surface.get());

		// Select appropriate format, present mode and extent
		this->surfaceFormat = FindSwapchainSurfaceFormat(surfaceDetails.formats);
		this->presentMode = FindSwapchainPresentMode(surfaceDetails.presentModes, preferredMode);
		this->extent = FindSwapchainExtent(surfaceDetails.capabilities, vk::Extent2D(100, 100));

		// Determine the number of images in the swapchain
		uint32_t imageCount = std::max(surfaceDetails.capabilities.minImageCount, imagesCount);
		if (surfaceDetails.capabilities.maxImageCount > 0 && imageCount > surfaceDetails.capabilities.maxImageCount)
			imageCount = surfaceDetails.capabilities.maxImageCount;

		// Configure swapchain creation parameters
		auto swapchainCreateInfo = vk::SwapchainCreateInfoKHR()
		                           .setSurface(surface.get())
		                           .setMinImageCount(imageCount)
		                           .setImageFormat(surfaceFormat.format)
		                           .setImageColorSpace(surfaceFormat.colorSpace)
		                           .setImageExtent(extent)
		                           .setImageArrayLayers(1)
		                           .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
		                           .setPreTransform(surfaceDetails.capabilities.currentTransform)
		                           .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
		                           .setPresentMode(presentMode)
		                           .setClipped(true)
		                           .setOldSwapchain(nullptr);

		uint32_t familyIndices[] = {queueFamilyIndices.graphicsFamilyIndex, queueFamilyIndices.presentFamilyIndex};

		// Configure image sharing mode based on queue family indices
		if (queueFamilyIndices.graphicsFamilyIndex != queueFamilyIndices.presentFamilyIndex)
		{
			swapchainCreateInfo
				.setImageSharingMode(vk::SharingMode::eConcurrent)
				.setQueueFamilyIndexCount(2)
				.setPQueueFamilyIndices(familyIndices);
		}
		else
		{
			swapchainCreateInfo
				.setImageSharingMode(vk::SharingMode::eExclusive);
		}

		// Create the swapchain
		this->swapchain = logicalDevice.createSwapchainKHRUnique(swapchainCreateInfo);

		// Get the swapchain images
		std::vector<vk::Image> swapchainImages = logicalDevice.getSwapchainImagesKHR(swapchain.get());

		// Create image views for all swapchain images
		this->images.clear();
		for (size_t imageIndex = 0; imageIndex < swapchainImages.size(); imageIndex++)
		{
			Image newbie;
			newbie.imageData = std::unique_ptr<ImageData>(new ImageData(
				swapchainImages[imageIndex], vk::ImageType::e2D, glm::vec3(extent.width, extent.height, 1), 1, 1,
				surfaceFormat.format, vk::ImageLayout::eUndefined));
			newbie.imageView = std::unique_ptr<ImageView>(
				new ImageView(logicalDevice, newbie.imageData.get(), 0, 1, 0, 1));

			this->images.emplace_back(std::move(newbie));
		}
	}

	Swapchain::SurfaceDetails Swapchain::GetSurfaceDetails(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
	{
		SurfaceDetails surfaceDetails;
		surfaceDetails.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
		surfaceDetails.formats = physicalDevice.getSurfaceFormatsKHR(surface);
		surfaceDetails.presentModes = physicalDevice.getSurfacePresentModesKHR(surface);

		return surfaceDetails;
	}

	vk::SurfaceFormatKHR Swapchain::FindSwapchainSurfaceFormat(
		const std::vector<vk::SurfaceFormatKHR>& availableFormats)
	{
		vk::SurfaceFormatKHR bestFormat = {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear};

		if (availableFormats.size() == 1 && availableFormats[0].format == vk::Format::eUndefined)
		{
			return bestFormat;
		}

		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == bestFormat.format && availableFormat.colorSpace == bestFormat.colorSpace)
				return availableFormat;
		}
		throw std::runtime_error("No suitable format found");
		return bestFormat;
	}

	vk::PresentModeKHR Swapchain::FindSwapchainPresentMode(const std::vector<vk::PresentModeKHR> availablePresentModes,
	                                                       vk::PresentModeKHR preferredMode)
	{
		for (const auto& availablePresentMode : availablePresentModes)
		{
			if (availablePresentMode == preferredMode)
				return availablePresentMode;
		}

		return vk::PresentModeKHR::eFifo;
	}

	vk::Extent2D Swapchain::FindSwapchainExtent(const vk::SurfaceCapabilitiesKHR& capabilities, vk::Extent2D windowSize)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}
		else
		{
			vk::Extent2D actualExtent = windowSize;

			actualExtent.width = std::max(capabilities.minImageExtent.width,
			                              std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height,
			                               std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}
}
