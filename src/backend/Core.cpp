#include "Core.h"

#include <iostream>

#include <vector>
#include <set>

#include "DescriptorSetCache.h"
#include "Image.h"
#include "PipelineCache.h"
#include "RenderGraph.h"
#include "Surface.h"
#include "Swapchain.h"

namespace lz
{
	Core::Core(const char** instance_extensions, const uint32_t instance_extensions_count,
	           const WindowDesc* compatible_window_desc,
	           const bool enable_debugging)
	{
		std::vector<const char*>
			res_instance_extensions(instance_extensions, instance_extensions + instance_extensions_count);
		std::vector<const char*> validation_layers;
		if (enable_debugging)
		{
			validation_layers.push_back("VK_LAYER_KHRONOS_validation");
			res_instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		this->instance_ = create_instance(res_instance_extensions, validation_layers);
		loader_ = vk::DispatchLoaderDynamic(instance_.get(), vkGetInstanceProcAddr);

		auto prop = vk::enumerateInstanceLayerProperties();

		if (enable_debugging)
		{
			this->debug_utils_messenger_ = create_debug_utils_messenger(instance_.get(), debug_message_callback,
			                                                            loader_);
		}
		this->physical_device_ = find_physical_device(instance_.get());

		// std::cout << "Supported extensions:\n";
		// auto extensions = physicalDevice.enumerateDeviceExtensionProperties();
		// for (auto extension : extensions)
		// {
		//   std::cout << "  " << extension.extensionName << "\n";
		// }

		if (compatible_window_desc)
		{
			vk::UniqueSurfaceKHR compatible_surface;
			if (!compatible_surface)
			{
				compatible_surface = create_win32_surface(instance_.get(), *compatible_window_desc);
			}
			this->queue_family_indices_ = find_queue_family_indices(physical_device_, compatible_surface.get());
		}

		std::vector<const char*> device_extensions;
		device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		this->logical_device_ = create_logical_device(physical_device_, queue_family_indices_, device_extensions,
		                                              validation_layers);
		this->graphics_queue_ = get_device_queue(logical_device_.get(), queue_family_indices_.graphics_family_index);
		this->present_queue_ = get_device_queue(logical_device_.get(), queue_family_indices_.present_family_index);
		this->command_pool_ = create_command_pool(logical_device_.get(), queue_family_indices_.graphics_family_index);

		this->descriptor_set_cache_.reset(new lz::DescriptorSetCache(logical_device_.get()));
		this->pipeline_cache_.reset(new lz::PipelineCache(logical_device_.get(), this->descriptor_set_cache_.get()));

		this->render_graph_.reset(new lz::RenderGraph(physical_device_, logical_device_.get(), loader_));
	}

	Core::~Core()
	{
	}

	void Core::clear_caches() const
	{
		this->descriptor_set_cache_->clear();
		this->pipeline_cache_->clear();
	}

	std::unique_ptr<Swapchain> Core::create_swapchain(WindowDesc window_desc, uint32_t images_count,
	                                                  vk::PresentModeKHR preferred_mode)
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

	void Core::set_debug_name(lz::ImageData* image_data, const std::string& name)
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

	lz::RenderGraph* Core::get_render_graph() const
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

	lz::DescriptorSetCache* Core::get_descriptor_set_cache() const
	{
		return descriptor_set_cache_.get();
	}

	lz::PipelineCache* Core::get_pipeline_cache() const
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

	vk::UniqueInstance Core::create_instance(const std::vector<const char*>& instance_extensions,
	                                         const std::vector<const char*>& validation_layers)
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
		const vk::DispatchLoaderDynamic& loader)
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
		                                   .setPUserData(nullptr); // Optional

		return instance.createDebugUtilsMessengerEXTUnique(messenger_create_info, nullptr, loader);
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL Core::debug_message_callback(
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
		VkDebugUtilsMessageTypeFlagsEXT message_type,
		const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
		void* p_user_data)
	{
		std::cerr << "validation layer: " << p_callback_data->pMessage << '\n';

		return VK_FALSE;
	}

	vk::PhysicalDevice Core::find_physical_device(const vk::Instance instance)
	{
		const std::vector<vk::PhysicalDevice> physical_devices = instance.enumeratePhysicalDevices();
		std::cout << "Found " << physical_devices.size() << " physical device(s)\n";
		vk::PhysicalDevice physical_device = nullptr;
		for (const auto& device : physical_devices)
		{
			vk::PhysicalDeviceProperties device_properties = device.getProperties();
			std::cout << "  Physical device found: " << device_properties.deviceName;
			vk::PhysicalDeviceFeatures device_features = device.getFeatures();

			// check if the device supports mesh shader extension
			bool mesh_shader_supported = false;
			auto extensions = device.enumerateDeviceExtensionProperties();
			for (const auto& extension : extensions)
			{
				if (strcmp(extension.extensionName, VK_EXT_MESH_SHADER_EXTENSION_NAME) == 0)
				{
					mesh_shader_supported_ = true;
					break;
				}
			}
			
			if (mesh_shader_supported_)
			{
				std::cout << " (Mesh Shader Supported)";
			}
			else
			{
				std::cout << " (Mesh Shader NOT Supported)";
			}

			if (device_properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
			{
				physical_device = device;
				std::cout << " <-- Using this device";
			}
			std::cout << "\n";
		}
		if (!physical_device)
			throw std::runtime_error("Failed to find physical device");
		return physical_device;
	}

	QueueFamilyIndices Core::find_queue_family_indices(const vk::PhysicalDevice physical_device,
	                                                   const vk::SurfaceKHR surface)
	{
		const std::vector<vk::QueueFamilyProperties> queue_families = physical_device.getQueueFamilyProperties();

		QueueFamilyIndices queue_family_indices;
		queue_family_indices.graphics_family_index = static_cast<uint32_t>(-1);
		queue_family_indices.present_family_index = static_cast<uint32_t>(-1);
		for (uint32_t family_index = 0; family_index < queue_families.size(); ++family_index)
		{
			if (queue_families[family_index].queueFlags & vk::QueueFlagBits::eGraphics && queue_families[family_index].
				queueCount > 0 && queue_family_indices.graphics_family_index == static_cast<uint32_t>(-1))
				queue_family_indices.graphics_family_index = family_index;

			if (physical_device.getSurfaceSupportKHR(family_index, surface) && queue_families[family_index].queueCount >
				0
				&& queue_family_indices.present_family_index == static_cast<uint32_t>(-1))
				queue_family_indices.present_family_index = family_index;
		}
		if (queue_family_indices.graphics_family_index == static_cast<uint32_t>(-1))
			throw std::runtime_error("Failed to find appropriate queue families");
		return queue_family_indices;
	}

	vk::UniqueDevice Core::create_logical_device(vk::PhysicalDevice physical_device, QueueFamilyIndices family_indices,
	                                             std::vector<const char*> device_extensions,
	                                             std::vector<const char*> validation_layers)
	{
		std::set<uint32_t> unique_queue_family_indices = {
			family_indices.graphics_family_index, family_indices.present_family_index
		};

		std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
		float queue_priority = 1.0f;
		for (uint32_t queue_family : unique_queue_family_indices)
		{
			auto queue_create_info = vk::DeviceQueueCreateInfo()
			                         .setQueueFamilyIndex(queue_family)
			                         .setQueueCount(1)
			                         .setPQueuePriorities(&queue_priority);

			queue_create_infos.push_back(queue_create_info);
		}

		// check if the device supports mesh shader extension
		auto extensions = physical_device.enumerateDeviceExtensionProperties();
		bool shader_draw_parameters_extension_supported = false;
		for (const auto& extension : extensions)
		{
			if (strcmp(extension.extensionName, VK_EXT_MESH_SHADER_EXTENSION_NAME) == 0)
			{
				mesh_shader_supported_ = true;
				// only add the extension if the device supports it
				device_extensions.push_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);
				std::cout << "Mesh Shader extension supported, adding to device extensions\n";
			}
			// Check for shader draw parameters extension
			if (strcmp(extension.extensionName, VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME) == 0)
			{
				shader_draw_parameters_extension_supported = true;
				device_extensions.push_back(VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME);
				std::cout << "Shader Draw Parameters extension supported, adding to device extensions\n";
			}
		}

		auto device_features = vk::PhysicalDeviceFeatures()
		                       .setFragmentStoresAndAtomics(true)
		                       .setVertexPipelineStoresAndAtomics(true)
		                       .setMultiDrawIndirect(true);  // Enable multiDrawIndirect feature

		auto device_create_info = vk::DeviceCreateInfo()
		                          .setQueueCreateInfoCount(uint32_t(queue_create_infos.size()))
		                          .setPQueueCreateInfos(queue_create_infos.data())
		                          .setPEnabledFeatures(&device_features)
		                          .setEnabledExtensionCount(uint32_t(device_extensions.size()))
		                          .setPpEnabledExtensionNames(device_extensions.data())
		                          .setEnabledLayerCount(uint32_t(validation_layers.size()))
		                          .setPpEnabledLayerNames(validation_layers.data());

		auto device_features12 = vk::PhysicalDeviceVulkan12Features()
			.setScalarBlockLayout(true)
			.setDrawIndirectCount(true)
			.setStorageBuffer8BitAccess(true);
			
		// Enable Vulkan 1.1 features including shaderDrawParameters
		auto device_features11 = vk::PhysicalDeviceVulkan11Features()
			.setShaderDrawParameters(true);

		if (mesh_shader_supported_)
		{
			// create mesh shader feature structure
			auto mesh_shader_features = vk::PhysicalDeviceMeshShaderFeaturesEXT()
				.setTaskShader(true)
				.setMeshShader(true);

			// build the structure chain, add mesh shader features and Vulkan 1.1 features
			vk::StructureChain<vk::DeviceCreateInfo, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan12Features, vk::PhysicalDeviceMeshShaderFeaturesEXT> chain = {
				device_create_info, device_features11, device_features12, mesh_shader_features
			};

			return physical_device.createDeviceUnique(chain.get<vk::DeviceCreateInfo>());
		}
		else
		{
			// not supported mesh shader, use the original feature chain with Vulkan 1.1 features
			vk::StructureChain<vk::DeviceCreateInfo, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan12Features> chain = {
				device_create_info, device_features11, device_features12
			};

			return physical_device.createDeviceUnique(chain.get<vk::DeviceCreateInfo>());
		}
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
}
