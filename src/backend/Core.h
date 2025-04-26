#pragma once
#include <iostream>

#include <vector>
#include <set>
#include "Surface.h"
#include "DescriptorSetCache.h"
#include "PipelineCache.h"
#include "QueueIndices.h"
#include "RenderGraph.h"


namespace lz
{
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
		Core(const char** instance_extensions, uint32_t instance_extensions_count,
		     const WindowDesc* compatible_window_desc,
		     bool enable_debugging);

		// Destructor: Cleans up Vulkan resources
		~Core();

		// ClearCaches: Resets all resource caches
		void clear_caches() const;

		// CreateSwapchain: Creates a new swapchain for rendering to a window
		// Parameters:
		// - windowDesc: Window descriptor for creating the swapchain
		// - imagesCount: Number of images in the swapchain
		// - preferredMode: Preferred presentation mode
		// Returns: Unique pointer to the created swapchain
		std::unique_ptr<Swapchain> create_swapchain(WindowDesc window_desc, uint32_t images_count,
		                                            vk::PresentModeKHR preferred_mode);

		// SetObjectDebugName: Sets a debug name for a Vulkan object
		// Parameters:
		// - logicalDevice: Vulkan logical device
		// - loader: Vulkan extension loader
		// - objHandle: Handle to the Vulkan object
		// - name: Debug name to set
		template <typename Handle, typename Loader>
		static void set_object_debug_name(vk::Device logical_device, Loader& loader, Handle obj_handle,
		                                  const std::string name)
		{
			auto nameInfo = vk::DebugUtilsObjectNameInfoEXT()
			                .setObjectHandle(uint64_t(Handle::CType(obj_handle)))
			                .setObjectType(obj_handle.objectType)
			                .setPObjectName(name.c_str());
			if (loader.vkSetDebugUtilsObjectNameEXT)
				auto res = logical_device.setDebugUtilsObjectNameEXT(&nameInfo, loader);
		}

		// SetObjectDebugName: Convenient overload for setting debug names
		template <typename T>
		void set_object_debug_name(T obj_handle, std::string name)
		{
			set_object_debug_name(logical_device_.get(), loader_, obj_handle, name);
		}

		// SetDebugName: Sets a debug name for an image resource
		void set_debug_name(lz::ImageData* image_data, const std::string& name);

		// GetCommandPool: Returns the command pool for allocating command buffers
		vk::CommandPool get_command_pool();

		// GetLogicalDevice: Returns the Vulkan logical device
		vk::Device get_logical_device();

		// GetPhysicalDevice: Returns the Vulkan physical device
		vk::PhysicalDevice get_physical_device() const;

		// GetRenderGraph: Returns the render graph for managing render passes
		lz::RenderGraph* get_render_graph() const;

		// AllocateCommandBuffers: Allocates command buffers from the command pool
		// Parameters:
		// - count: Number of command buffers to allocate
		// Returns: Vector of allocated command buffers
		std::vector<vk::UniqueCommandBuffer> allocate_command_buffers(size_t count);

		// CreateVulkanSemaphore: Creates a new Vulkan semaphore
		vk::UniqueSemaphore create_vulkan_semaphore();

		// CreateFence: Creates a new Vulkan fence
		// Parameters:
		// - state: Initial state of the fence (true = signaled, false = unsignaled)
		vk::UniqueFence create_fence(bool state);

		// WaitForFence: Blocks until the specified fence is signaled
		void wait_for_fence(vk::Fence fence);

		// ResetFence: Resets the specified fence to unsignaled state
		void reset_fence(vk::Fence fence);

		// WaitIdle: Waits for all device operations to complete
		void wait_idle();

		// GetGraphicsQueue: Returns the graphics queue for submitting commands
		vk::Queue get_graphics_queue() const;

		// GetPresentQueue: Returns the presentation queue
		vk::Queue get_present_queue() const;

		// GetDynamicMemoryAlignment: Returns the required alignment for dynamic memory
		uint32_t get_dynamic_memory_alignment() const;

		// GetDescriptorSetCache: Returns the descriptor set cache
		lz::DescriptorSetCache* get_descriptor_set_cache() const;

		// GetPipelineCache: Returns the pipeline cache
		lz::PipelineCache* get_pipeline_cache() const;

		// check if the device supports mesh shader extension
		bool mesh_shader_supported() const;

		vk::DispatchLoaderDynamic get_dynamic_loader() const;

	private:
		// CreateInstance: Creates a Vulkan instance with specified extensions and layers
		vk::UniqueInstance create_instance(const std::vector<const char*>& instance_extensions,
		                                          const std::vector<const char*>& validation_layers);

		// CreateDebugUtilsMessenger: Sets up the debug messenger for validation layers
		static vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> create_debug_utils_messenger(
			vk::Instance instance, PFN_vkDebugUtilsMessengerCallbackEXT debug_callback,
			const vk::DispatchLoaderDynamic& loader);

		// DebugMessageCallback: Callback function for Vulkan validation messages
		static VKAPI_ATTR VkBool32 VKAPI_CALL debug_message_callback(
			VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
			VkDebugUtilsMessageTypeFlagsEXT message_type,
			const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
			void* p_user_data);

		friend class Swapchain;

		// FindPhysicalDevice: Selects an appropriate physical device
		vk::PhysicalDevice find_physical_device(vk::Instance instance);

		// FindQueueFamilyIndices: Finds queue families for graphics and presentation
		QueueFamilyIndices find_queue_family_indices(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface);

		// CreateLogicalDevice: Creates a logical device with required features and queues
		vk::UniqueDevice create_logical_device(vk::PhysicalDevice physical_device,
		                                              QueueFamilyIndices family_indices,
		                                              std::vector<const char*> device_extensions,
		                                              std::vector<const char*> validation_layers);

		// GetDeviceQueue: Retrieves a queue from the logical device
		vk::Queue get_device_queue(vk::Device logical_device, uint32_t queue_family_index);

		// CreateCommandPool: Creates a command pool for allocating command buffers
		vk::UniqueCommandPool create_command_pool(vk::Device logical_device, uint32_t family_index);

		// check if the device supports mesh shader extension
		bool mesh_shader_supported_;

		// Core Vulkan objects
		vk::UniqueInstance instance_;
		vk::DispatchLoaderDynamic loader_;
		vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> debug_utils_messenger_;
		vk::PhysicalDevice physical_device_;
		vk::UniqueDevice logical_device_;
		vk::UniqueCommandPool command_pool_;
		vk::Queue graphics_queue_;
		vk::Queue present_queue_;

		// Resource caches and managers
		std::unique_ptr<lz::DescriptorSetCache> descriptor_set_cache_;
		std::unique_ptr<lz::PipelineCache> pipeline_cache_;
		std::unique_ptr<lz::RenderGraph> render_graph_;

		QueueFamilyIndices queue_family_indices_;
	};
}
