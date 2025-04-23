#pragma once
#include <iostream>

#include <vector>
#include <set>

namespace lz
{
	class Swapchain;
	class RenderGraph;

	// Core: Main engine class that manages Vulkan instance, devices, and core resources
	class Core
	{
	public:
		// Constructor: Initializes Vulkan instance and core components
		// Parameters:
		// - instanceExtensions: Required Vulkan instance extensions
		// - instanceExtensionsCount: Number of instance extensions
		// - compatibleWindowDesc: Window descriptor for surface compatibility check
		// - enableDebugging: Whether to enable Vulkan validation layers
		Core(const char** instanceExtensions, uint32_t instanceExtensionsCount, WindowDesc* compatibleWindowDesc,
		     bool enableDebugging);

		// Destructor: Cleans up Vulkan resources
		~Core();
		
		// ClearCaches: Resets all resource caches
		void ClearCaches();
		
		// CreateSwapchain: Creates a new swapchain for rendering to a window
		// Parameters:
		// - windowDesc: Window descriptor for creating the swapchain
		// - imagesCount: Number of images in the swapchain
		// - preferredMode: Preferred presentation mode
		// Returns: Unique pointer to the created swapchain
		std::unique_ptr<Swapchain> CreateSwapchain(WindowDesc windowDesc, uint32_t imagesCount,
		                                           vk::PresentModeKHR preferredMode);

		// SetObjectDebugName: Sets a debug name for a Vulkan object
		// Parameters:
		// - logicalDevice: Vulkan logical device
		// - loader: Vulkan extension loader
		// - objHandle: Handle to the Vulkan object
		// - name: Debug name to set
		template <typename Handle, typename Loader>
		static void SetObjectDebugName(vk::Device logicalDevice, Loader& loader, Handle objHandle, std::string name)
		{
			auto nameInfo = vk::DebugUtilsObjectNameInfoEXT()
			                .setObjectHandle(uint64_t(Handle::CType(objHandle)))
			                .setObjectType(objHandle.objectType)
			                .setPObjectName(name.c_str());
			if (loader.vkSetDebugUtilsObjectNameEXT)
				auto res = logicalDevice.setDebugUtilsObjectNameEXT(&nameInfo, loader);
		}

		// SetObjectDebugName: Convenient overload for setting debug names
		template <typename T>
		void SetObjectDebugName(T objHandle, std::string name)
		{
			SetObjectDebugName(logicalDevice.get(), loader, objHandle, name);
		}

		// SetDebugName: Sets a debug name for an image resource
		void SetDebugName(lz::ImageData* imageData, std::string name);

		// GetCommandPool: Returns the command pool for allocating command buffers
		vk::CommandPool GetCommandPool();
		
		// GetLogicalDevice: Returns the Vulkan logical device
		vk::Device GetLogicalDevice();
		
		// GetPhysicalDevice: Returns the Vulkan physical device
		vk::PhysicalDevice GetPhysicalDevice();
		
		// GetRenderGraph: Returns the render graph for managing render passes
		lz::RenderGraph* GetRenderGraph();
		
		// AllocateCommandBuffers: Allocates command buffers from the command pool
		// Parameters:
		// - count: Number of command buffers to allocate
		// Returns: Vector of allocated command buffers
		std::vector<vk::UniqueCommandBuffer> AllocateCommandBuffers(size_t count);
		
		// CreateVulkanSemaphore: Creates a new Vulkan semaphore
		vk::UniqueSemaphore CreateVulkanSemaphore();
		
		// CreateFence: Creates a new Vulkan fence
		// Parameters:
		// - state: Initial state of the fence (true = signaled, false = unsignaled)
		vk::UniqueFence CreateFence(bool state);
		
		// WaitForFence: Blocks until the specified fence is signaled
		void WaitForFence(vk::Fence fence);
		
		// ResetFence: Resets the specified fence to unsignaled state
		void ResetFence(vk::Fence fence);
		
		// WaitIdle: Waits for all device operations to complete
		void WaitIdle();
		
		// GetGraphicsQueue: Returns the graphics queue for submitting commands
		vk::Queue GetGraphicsQueue();
		
		// GetPresentQueue: Returns the presentation queue
		vk::Queue GetPresentQueue();
		
		// GetDynamicMemoryAlignment: Returns the required alignment for dynamic memory
		uint32_t GetDynamicMemoryAlignment();
		
		// GetDescriptorSetCache: Returns the descriptor set cache
		lz::DescriptorSetCache* GetDescriptorSetCache();
		
		// GetPipelineCache: Returns the pipeline cache
		lz::PipelineCache* GetPipelineCache();

	private:
		// CreateInstance: Creates a Vulkan instance with specified extensions and layers
		static vk::UniqueInstance CreateInstance(const std::vector<const char*>& instanceExtensions,
		                                         const std::vector<const char*>& validationLayers);

		// CreateDebugUtilsMessenger: Sets up the debug messenger for validation layers
		static vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> CreateDebugUtilsMessenger(
			vk::Instance instance, PFN_vkDebugUtilsMessengerCallbackEXT debugCallback,
			vk::DispatchLoaderDynamic& loader);

		// DebugMessageCallback: Callback function for Vulkan validation messages
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessageCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);

		friend class Swapchain;
		
		// FindPhysicalDevice: Selects an appropriate physical device
		static vk::PhysicalDevice FindPhysicalDevice(vk::Instance instance);

		// FindQueueFamilyIndices: Finds queue families for graphics and presentation
		static QueueFamilyIndices FindQueueFamilyIndices(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface);

		// CreateLogicalDevice: Creates a logical device with required features and queues
		static vk::UniqueDevice CreateLogicalDevice(vk::PhysicalDevice physicalDevice, QueueFamilyIndices familyIndices,
		                                            std::vector<const char*> deviceExtensions,
		                                            std::vector<const char*> validationLayers);
		
		// GetDeviceQueue: Retrieves a queue from the logical device
		static vk::Queue GetDeviceQueue(vk::Device logicalDevice, uint32_t queueFamilyIndex);
		
		// CreateCommandPool: Creates a command pool for allocating command buffers
		static vk::UniqueCommandPool CreateCommandPool(vk::Device logicalDevice, uint32_t familyIndex);

		// Core Vulkan objects
		vk::UniqueInstance instance;
		vk::DispatchLoaderDynamic loader;
		vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> debugUtilsMessenger;
		vk::PhysicalDevice physicalDevice;
		vk::UniqueDevice logicalDevice;
		vk::UniqueCommandPool commandPool;
		vk::Queue graphicsQueue;
		vk::Queue presentQueue;

		// Resource caches and managers
		std::unique_ptr<lz::DescriptorSetCache> descriptorSetCache;
		std::unique_ptr<lz::PipelineCache> pipelineCache;
		std::unique_ptr<lz::RenderGraph> renderGraph;

		QueueFamilyIndices queueFamilyIndices;
	};
}
