#pragma once
#include <iostream>

#include <vector>
#include <set>
namespace lz
{
	inline Core::Core(const char** instanceExtensions, uint32_t instanceExtensionsCount, WindowDesc* compatibleWindowDesc, bool enableDebugging)
	{
		std::vector<const char*> resInstanceExtensions(instanceExtensions, instanceExtensions + instanceExtensionsCount);
		std::vector<const char*> validationLayers;
		if (enableDebugging)
		{
			validationLayers.push_back("VK_LAYER_KHRONOS_validation");
			resInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		this->instance = CreateInstance(resInstanceExtensions, validationLayers);
		loader = vk::DispatchLoaderDynamic(instance.get(), vkGetInstanceProcAddr);

		auto prop = vk::enumerateInstanceLayerProperties();

		if (enableDebugging)
		{
			this->debugUtilsMessenger = CreateDebugUtilsMessenger(instance.get(), DebugMessageCallback, loader);
		}
		this->physicalDevice = FindPhysicalDevice(instance.get());

		/*std::cout << "Supported extensions:\n";
		auto extensions = physicalDevice.enumerateDeviceExtensionProperties();
		for (auto extension : extensions)
		{
		  std::cout << "  " << extension.extensionName << "\n";
		}*/

		if (compatibleWindowDesc)
		{
			vk::UniqueSurfaceKHR compatibleSurface;
			if (compatibleSurface)
			{
				compatibleSurface = CreateWin32Surface(instance.get(), *compatibleWindowDesc);
			}
			this->queueFamilyIndices = FindQueueFamilyIndices(physicalDevice, compatibleSurface.get());
		}

		std::vector<const char*> deviceExtensions;
		deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		this->logicalDevice = CreateLogicalDevice(physicalDevice, queueFamilyIndices, deviceExtensions, validationLayers);
		this->graphicsQueue = GetDeviceQueue(logicalDevice.get(), queueFamilyIndices.graphicsFamilyIndex);
		this->presentQueue = GetDeviceQueue(logicalDevice.get(), queueFamilyIndices.presentFamilyIndex);
		this->commandPool = CreateCommandPool(logicalDevice.get(), queueFamilyIndices.graphicsFamilyIndex);

		this->descriptorSetCache.reset(new lz::DescriptorSetCache(logicalDevice.get()));
		this->pipelineCache.reset(new lz::PipelineCache(logicalDevice.get(), this->descriptorSetCache.get()));

		//this->renderGraph.reset(new lz::RenderGraph(physicalDevice, logicalDevice.get(), loader));

	}

	inline Core::~Core()
	{
		
	}

	inline void Core::ClearCaches()
	{
		this->descriptorSetCache->Clear();
		this->pipelineCache->Clear();
	}





}