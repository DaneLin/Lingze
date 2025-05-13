#include "Core.h"

#include <iostream>

#include <set>
#include <vector>

#include "DescriptorSetCache.h"
#include "Image.h"
#include "Logging.h"
#include "PipelineCache.h"
#include "RenderGraph.h"
#include "Surface.h"
#include "Swapchain.h"

namespace lz
{
Core::Core(const char **instance_extensions, const uint32_t instance_extensions_count,
           const WindowDesc                *compatible_window_desc,
           const bool                       enable_debugging,
           const std::vector<const char *> &device_extensions_input)
{
	std::vector<const char *> res_instance_extensions(instance_extensions, instance_extensions + instance_extensions_count);
	std::vector<const char *> validation_layers;
	if (enable_debugging)
	{
		validation_layers.push_back("VK_LAYER_KHRONOS_validation");
		res_instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	instance_ = create_instance(res_instance_extensions, validation_layers);
	loader_   = vk::DispatchLoaderDynamic(instance_.get(), vkGetInstanceProcAddr);

	auto prop = vk::enumerateInstanceLayerProperties();

	if (enable_debugging)
	{
		this->debug_utils_messenger_ = create_debug_utils_messenger(instance_.get(), debug_message_callback,
		                                                            loader_);
	}
	this->physical_device_ = find_physical_device(instance_.get());

	if (compatible_window_desc)
	{
		vk::UniqueSurfaceKHR compatible_surface;
		if (!compatible_surface)
		{
			compatible_surface = create_win32_surface(instance_.get(), *compatible_window_desc);
		}
		this->queue_family_indices_ = find_queue_family_indices(physical_device_, compatible_surface.get());
	}

	// Initialize device extensions with input list
	std::vector<const char *> device_extensions = device_extensions_input;

	// Ensure swapchain extension is included
	bool has_swapchain = false;
	for (const auto &ext : device_extensions)
	{
		if (strcmp(ext, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
		{
			has_swapchain = true;
			break;
		}
	}

	if (!has_swapchain)
	{
		device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	}

	this->logical_device_ = create_logical_device(physical_device_, queue_family_indices_, device_extensions, validation_layers);
	this->graphics_queue_ = get_device_queue(logical_device_.get(), queue_family_indices_.graphics_family_index);
	this->present_queue_  = get_device_queue(logical_device_.get(), queue_family_indices_.present_family_index);
	this->command_pool_   = create_command_pool(logical_device_.get(), queue_family_indices_.graphics_family_index);

	this->descriptor_set_cache_.reset(new lz::DescriptorSetCache(logical_device_.get(), bindless_supported_));
	this->pipeline_cache_.reset(new lz::PipelineCache(logical_device_.get(), this->descriptor_set_cache_.get()));
	this->render_graph_.reset(new lz::RenderGraph(physical_device_, logical_device_.get(), loader_));

	if (bindless_supported_)
	{
		this->material_system_.reset(new lz::MaterialSystem(this));
	}
}

Core::~Core()
{
}

void Core::clear_caches() const
{
	this->descriptor_set_cache_->clear();
	this->pipeline_cache_->clear();
}

std::unique_ptr<Swapchain> Core::create_swapchain(WindowDesc window_desc, uint32_t images_count, vk::PresentModeKHR preferred_mode)
{
	// auto swapchain = std::make_unique<Swapchain>(instance_.get(), physical_device_, logical_device_.get(),
	//                                              window_desc, images_count, queue_family_indices_,
	//                                              preferred_mode);

	auto swapchain = std::unique_ptr<Swapchain>(new Swapchain(instance_.get(), physical_device_,
	                                                          logical_device_.get(),
	                                                          window_desc, images_count, queue_family_indices_,
	                                                          preferred_mode));
	return swapchain;
}

void Core::set_debug_name(lz::ImageData *image_data, const std::string &name)
{
	set_object_debug_name(image_data->get_handle(), name);
	image_data->set_debug_name(name);
}

vk::CommandPool Core::get_command_pool()
{
	return command_pool_.get();
}

vk::Device Core::get_logical_device()
{
	return logical_device_.get();
}

vk::PhysicalDevice Core::get_physical_device() const
{
	return physical_device_;
}

lz::RenderGraph *Core::get_render_graph() const
{
	return render_graph_.get();
}

std::vector<vk::UniqueCommandBuffer> Core::allocate_command_buffers(size_t count)
{
	const auto command_buffer_allocate_info = vk::CommandBufferAllocateInfo()
	                                              .setCommandPool(command_pool_.get())
	                                              .setLevel(vk::CommandBufferLevel::ePrimary)
	                                              .setCommandBufferCount(uint32_t(count));

	return logical_device_->allocateCommandBuffersUnique(command_buffer_allocate_info);
}

vk::UniqueSemaphore Core::create_vulkan_semaphore()
{
	auto semaphore_info = vk::SemaphoreCreateInfo();
	return logical_device_->createSemaphoreUnique(semaphore_info);
}

vk::UniqueFence Core::create_fence(bool state)
{
	auto fence_info = vk::FenceCreateInfo();
	if (state)
	{
		fence_info.setFlags(vk::FenceCreateFlagBits::eSignaled);
	}
	return logical_device_->createFenceUnique(fence_info);
}

void Core::wait_for_fence(vk::Fence fence)
{
	auto res = logical_device_->waitForFences({fence}, true, std::numeric_limits<uint64_t>::max());
}

void Core::reset_fence(vk::Fence fence)
{
	logical_device_->resetFences({fence});
}

void Core::wait_idle()
{
	logical_device_->waitIdle();
}

vk::Queue Core::get_graphics_queue() const
{
	return graphics_queue_;
}

vk::Queue Core::get_present_queue() const
{
	return present_queue_;
}

uint32_t Core::get_dynamic_memory_alignment() const
{
	return static_cast<uint32_t>(physical_device_.getProperties().limits.minUniformBufferOffsetAlignment);
}

lz::DescriptorSetCache *Core::get_descriptor_set_cache() const
{
	return descriptor_set_cache_.get();
}

lz::PipelineCache *Core::get_pipeline_cache() const
{
	return pipeline_cache_.get();
}

bool Core::mesh_shader_supported() const
{
	return mesh_shader_supported_;
}

vk::DispatchLoaderDynamic Core::get_dynamic_loader() const
{
	return loader_;
}

bool Core::bindless_supported() const
{
	return bindless_supported_;
}

void Core::register_material(const std::shared_ptr<lz::Material> &material)	
{
	if (material_system_)
	{
		material_system_->register_material(material);
	}
}

void Core::process_pending_material_updates()
{
	if (material_system_)
	{
		material_system_->process_pending_updates();
	}
}

vk::UniqueInstance Core::create_instance(const std::vector<const char *> &instance_extensions, const std::vector<const char *> &validation_layers)
{
	constexpr auto app_info = vk::ApplicationInfo()
	                              .setPApplicationName("Lingze app")
	                              .setApplicationVersion(VK_MAKE_VERSION(-1, 0, 0))
	                              .setPEngineName("Lingze engine")
	                              .setEngineVersion(VK_MAKE_VERSION(-1, 0, 0))
	                              .setApiVersion(VK_API_VERSION_1_2);

	const auto instance_create_info = vk::InstanceCreateInfo()
	                                      .setPApplicationInfo(&app_info)
	                                      .setEnabledExtensionCount(uint32_t(instance_extensions.size()))
	                                      .setPpEnabledExtensionNames(instance_extensions.data())
	                                      .setEnabledLayerCount(uint32_t(validation_layers.size()))
	                                      .setPpEnabledLayerNames(validation_layers.data());

	return vk::createInstanceUnique(instance_create_info);
}

vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> Core::create_debug_utils_messenger(
    const vk::Instance instance, const PFN_vkDebugUtilsMessengerCallbackEXT debug_callback,
    const vk::DispatchLoaderDynamic &loader)
{
	const auto messenger_create_info = vk::DebugUtilsMessengerCreateInfoEXT()
	                                       .setMessageSeverity(
	                                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
	                                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
	                                           /* | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo*/)
	                                       .setMessageType(
	                                           vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
	                                           vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
	                                           vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
	                                       .setPfnUserCallback(debug_callback)
	                                       .setPUserData(nullptr);        // Optional

	return instance.createDebugUtilsMessengerEXTUnique(messenger_create_info, nullptr, loader);
}

VKAPI_ATTR VkBool32 VKAPI_CALL Core::debug_message_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
    VkDebugUtilsMessageTypeFlagsEXT             message_type,
    const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data,
    void                                       *p_user_data)
{
	// std::cerr << "validation layer: " << p_callback_data->pMessage << '\n';
	LOGI("validation layer : {}", p_callback_data->pMessage);

	return VK_FALSE;
}

vk::PhysicalDevice Core::find_physical_device(const vk::Instance instance)
{
	const std::vector<vk::PhysicalDevice> physical_devices = instance.enumeratePhysicalDevices();
	LOGI("Found {} physical device(s)", physical_devices.size());

	if (physical_devices.empty())
	{
		throw std::runtime_error("Failed to find any GPU with Vulkan support");
	}

	// Rate each physical device and select the best one
	struct DeviceCandidate
	{
		vk::PhysicalDevice device;
		int                score;
	};

	std::vector<DeviceCandidate> candidates;

	// Required device extensions
	std::vector<const char *> required_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

	for (const auto &device : physical_devices)
	{
		vk::PhysicalDeviceProperties       device_properties = device.getProperties();
		vk::PhysicalDeviceFeatures         device_features   = device.getFeatures();
		vk::PhysicalDeviceMemoryProperties memory_properties = device.getMemoryProperties();

		LOGI("  Physical device found : {}", device_properties.deviceName.data());

		// Check if device supports graphics queue family
		const std::vector<vk::QueueFamilyProperties> queue_families     = device.getQueueFamilyProperties();
		bool                                         has_graphics_queue = false;

		for (const auto &queue_family : queue_families)
		{
			if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics)
			{
				has_graphics_queue = true;
				break;
			}
		}

		if (!has_graphics_queue)
		{
			LOGI("    Device does not support graphics operations. Skipping.");
			continue;
		}

		// Check device extension support
		auto                  available_extensions = device.enumerateDeviceExtensionProperties();
		std::set<std::string> required_extensions_set(required_extensions.begin(), required_extensions.end());

		for (const auto &extension : available_extensions)
		{
			required_extensions_set.erase(extension.extensionName);
		}

		if (!required_extensions_set.empty())
		{
			LOGI("    Device does not support required extensions. Skipping.");
			continue;
		}

		// Initialize device score
		int score = 0;

		// Score based on device type
		switch (device_properties.deviceType)
		{
			case vk::PhysicalDeviceType::eDiscreteGpu:
				score += 1000;        // Discrete GPU gets highest score
				break;
			case vk::PhysicalDeviceType::eIntegratedGpu:
				score += 500;        // Integrated GPU gets second highest
				break;
			case vk::PhysicalDeviceType::eVirtualGpu:
				score += 200;
				break;
			case vk::PhysicalDeviceType::eCpu:
				score += 100;
				break;
			default:
				score += 10;
				break;
		}

		// Score based on device limits
		score += device_properties.limits.maxImageDimension2D / 1024;        // Image dimension limits

		// Score based on maximum compute workgroup count
		score += device_properties.limits.maxComputeWorkGroupCount[0] / 64;

		// Consider maximum texture size
		score += device_properties.limits.maxImageArrayLayers / 8;

		// Consider driver version
		score += device_properties.driverVersion / 10000;

		// Consider Vulkan API version
		score += VK_VERSION_MAJOR(device_properties.apiVersion) * 10;
		score += VK_VERSION_MINOR(device_properties.apiVersion) * 5;

		// consider device memory
		uint64_t device_memory = 0;
		for (uint32_t i = 0; i < memory_properties.memoryHeapCount; i++)
		{
			if (memory_properties.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal)
			{
				device_memory = memory_properties.memoryHeaps[i].size;
				score += static_cast<int>(device_memory / (1024 * 1024 * 1024));        // Add 1 point per GB of VRAM
				break;
			}
		}

		// convert device type to string for display
		std::string device_type_str;
		switch (device_properties.deviceType)
		{
			case vk::PhysicalDeviceType::eDiscreteGpu:
				device_type_str = "Discrete GPU";
				break;
			case vk::PhysicalDeviceType::eIntegratedGpu:
				device_type_str = "Integrated GPU";
				break;
			case vk::PhysicalDeviceType::eVirtualGpu:
				device_type_str = "Virtual GPU";
				break;
			case vk::PhysicalDeviceType::eCpu:
				device_type_str = "CPU";
				break;
			case vk::PhysicalDeviceType::eOther:
				device_type_str = "Other";
				break;
			default:
				device_type_str = "Unknown";
				break;
		}

		LOGI("    Type: {}, Memory: {} MB, Score: {}",
		     device_type_str,
		     (device_memory / (1024 * 1024)),
		     score);

		candidates.push_back({device, score});
	}

	// sort the candidates by score in descending order
	std::sort(candidates.begin(), candidates.end(),
	          [](const DeviceCandidate &a, const DeviceCandidate &b) {
		          return a.score > b.score;
	          });

	// select the device with the highest score
	if (candidates.empty())
	{
		throw std::runtime_error("Failed to find suitable physical device");
	}

	vk::PhysicalDevice                 best_device     = candidates[0].device;
	vk::PhysicalDeviceProperties       best_properties = best_device.getProperties();
	vk::PhysicalDeviceMemoryProperties best_memory     = best_device.getMemoryProperties();

	// get device type string
	std::string device_type_str;
	switch (best_properties.deviceType)
	{
		case vk::PhysicalDeviceType::eDiscreteGpu:
			device_type_str = "Discrete GPU";
			break;
		case vk::PhysicalDeviceType::eIntegratedGpu:
			device_type_str = "Integrated GPU";
			break;
		case vk::PhysicalDeviceType::eVirtualGpu:
			device_type_str = "Virtual GPU";
			break;
		case vk::PhysicalDeviceType::eCpu:
			device_type_str = "CPU";
			break;
		case vk::PhysicalDeviceType::eOther:
			device_type_str = "Other";
			break;
		default:
			device_type_str = "Unknown";
			break;
	}

	// calculate device local memory
	uint64_t device_local_memory = 0;
	for (uint32_t i = 0; i < best_memory.memoryHeapCount; i++)
	{
		if (best_memory.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal)
		{
			device_local_memory += best_memory.memoryHeaps[i].size;
		}
	}

	LOGI("Selected physical device: {} (Score: {})", best_properties.deviceName.data(), candidates[0].score);
	LOGI("  Device type: {}", device_type_str);
	LOGI("  API version: {}.{}.{}",
	     VK_VERSION_MAJOR(best_properties.apiVersion),
	     VK_VERSION_MINOR(best_properties.apiVersion),
	     VK_VERSION_PATCH(best_properties.apiVersion));
	LOGI("  Driver version: {}", best_properties.driverVersion);
	LOGI("  Vendor ID: {}", best_properties.vendorID);
	LOGI("  Device ID: {}", best_properties.deviceID);
	LOGI("  Device local memory: {} MB", (device_local_memory / (1024 * 1024)));

	return best_device;
}

QueueFamilyIndices Core::find_queue_family_indices(const vk::PhysicalDevice physical_device, const vk::SurfaceKHR surface)
{
	const std::vector<vk::QueueFamilyProperties> queue_families = physical_device.getQueueFamilyProperties();

	QueueFamilyIndices queue_family_indices;
	queue_family_indices.graphics_family_index = static_cast<uint32_t>(-1);
	queue_family_indices.present_family_index  = static_cast<uint32_t>(-1);
	for (uint32_t family_index = 0; family_index < queue_families.size(); ++family_index)
	{
		if (queue_families[family_index].queueFlags & vk::QueueFlagBits::eGraphics && queue_families[family_index].queueCount > 0 && queue_family_indices.graphics_family_index == static_cast<uint32_t>(-1))
			queue_family_indices.graphics_family_index = family_index;

		if (physical_device.getSurfaceSupportKHR(family_index, surface) && queue_families[family_index].queueCount > 0 && queue_family_indices.present_family_index == static_cast<uint32_t>(-1))
			queue_family_indices.present_family_index = family_index;
	}
	if (queue_family_indices.graphics_family_index == static_cast<uint32_t>(-1))
		throw std::runtime_error("Failed to find appropriate queue families");
	return queue_family_indices;
}

vk::UniqueDevice Core::create_logical_device(vk::PhysicalDevice physical_device, QueueFamilyIndices family_indices,
                                             std::vector<const char *> device_extensions,
                                             std::vector<const char *> validation_layers)
{
	std::set<uint32_t> unique_queue_family_indices = {family_indices.graphics_family_index, family_indices.present_family_index};

	std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
	float                                  queue_priority = 1.0f;
	for (uint32_t queue_family : unique_queue_family_indices)
	{
		auto queue_create_info = vk::DeviceQueueCreateInfo()
		                             .setQueueFamilyIndex(queue_family)
		                             .setQueueCount(1)
		                             .setPQueuePriorities(&queue_priority);

		queue_create_infos.push_back(queue_create_info);
	}

	LOGI("Device extensions will be enabled:");
	for (const auto &ext : device_extensions)
	{
		LOGI("  {}", ext);
	}

	// Get all supported device extensions
	auto supported_extensions = physical_device.enumerateDeviceExtensionProperties();

	// Check if requested extensions are supported
	std::vector<const char *> enabled_extensions;
	for (const auto &ext_name : device_extensions)
	{
		bool extension_found = false;
		for (const auto &supported_ext : supported_extensions)
		{
			if (strcmp(supported_ext.extensionName, ext_name) == 0)
			{
				extension_found = true;
				enabled_extensions.push_back(ext_name);

				if (strcmp(ext_name, "VK_EXT_mesh_shader") == 0)
				{
					mesh_shader_supported_ = true;
				}

				if (strcmp(ext_name, "VK_EXT_descriptor_indexing") == 0)
				{
					bindless_supported_ = true;
				}
				break;
			}
		}

		if (!extension_found)
		{
			throw std::runtime_error("Device extension " + std::string(ext_name) + " is not supported by the physical device");
		}
	}

	// Request physical device features
	vk::PhysicalDeviceFeatures device_features;
	device_features.setMultiDrawIndirect(true);

	vk::PhysicalDeviceVulkan12Features device_vulkan12_features;
	device_vulkan12_features.setScalarBlockLayout(true);
	device_vulkan12_features.setDrawIndirectCount(true);
	device_vulkan12_features.setStorageBuffer8BitAccess(true);

	if (bindless_supported_)
	{
		device_vulkan12_features.setDescriptorIndexing(true);
		device_vulkan12_features.setDescriptorBindingVariableDescriptorCount(true);
		device_vulkan12_features.setDescriptorBindingPartiallyBound(true);
		device_vulkan12_features.setDescriptorBindingSampledImageUpdateAfterBind(true);
		device_vulkan12_features.setDescriptorBindingStorageImageUpdateAfterBind(true);
		device_vulkan12_features.setDescriptorBindingUniformBufferUpdateAfterBind(true);
		device_vulkan12_features.setDescriptorBindingStorageBufferUpdateAfterBind(true);
		device_vulkan12_features.setRuntimeDescriptorArray(true);
	}

	// Setup mesh shader feature structure if needed

	// Setup device create info
	vk::DeviceCreateInfo device_create_info;
	device_create_info.setQueueCreateInfoCount(static_cast<uint32_t>(queue_create_infos.size()))
	    .setPQueueCreateInfos(queue_create_infos.data())
	    .setPEnabledFeatures(&device_features)
	    .setEnabledExtensionCount(static_cast<uint32_t>(enabled_extensions.size()))
	    .setPpEnabledExtensionNames(enabled_extensions.data())
	    .setEnabledLayerCount(static_cast<uint32_t>(validation_layers.size()))
	    .setPpEnabledLayerNames(validation_layers.data());

	// Add mesh shader feature to pNext chain if enabled
	vk::PhysicalDeviceMeshShaderFeaturesEXT mesh_shader_features;
	mesh_shader_features.setMeshShader(true);
	mesh_shader_features.setTaskShader(true);

	void *pNext = &device_vulkan12_features;
	if (mesh_shader_supported_)
	{
		
		mesh_shader_features.pNext = pNext;
		pNext                      = &mesh_shader_features;
	}
	device_create_info.setPNext(pNext);

	return physical_device.createDeviceUnique(device_create_info);
}

vk::Queue Core::get_device_queue(const vk::Device logical_device, const uint32_t queue_family_index)
{
	return logical_device.getQueue(queue_family_index, 0);
}

vk::UniqueCommandPool Core::create_command_pool(const vk::Device logical_device, const uint32_t family_index)
{
	const auto command_pool_info = vk::CommandPoolCreateInfo()
	                                   //.setFlags(vk::CommandPoolCreateFlagBits::eTransient)
	                                   .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
	                                   .setQueueFamilyIndex(family_index);
	return logical_device.createCommandPoolUnique(command_pool_info);
}

const vk::UniqueDescriptorSet *Core::get_bindless_descriptor_set() const
{
	return material_system_->get_bindless_descriptor_set();
}

uint32_t Core::get_material_index(const std::string &material_name) const
{
	return material_system_->get_material_index(material_name);
}

lz::Buffer* Core::get_material_parameters_buffer() const
{
	return material_system_->get_material_parameters_buffer();
}

}        // namespace lz
