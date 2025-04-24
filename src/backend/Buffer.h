#pragma once

#include "LingzeVK.h"

namespace lz
{
	// FindMemoryTypeIndex: Finds a suitable memory type index for a buffer allocation
	// Parameters:
	// - physicalDevice: Physical device to query memory properties from
	// - suitableIndices: Bit field of suitable memory type indices
	// - memoryVisibility: Required memory property flags
	// Returns: Index of a suitable memory type, or -1 if none found
	static uint32_t find_memory_type_index(const vk::PhysicalDevice physical_device, const uint32_t suitable_indices,
	                                       const vk::MemoryPropertyFlags memory_visibility)
	{
		const vk::PhysicalDeviceMemoryProperties available_mem_properties = physical_device.getMemoryProperties();

		for (uint32_t i = 0; i < available_mem_properties.memoryTypeCount; ++i)
		{
			if ((suitable_indices & (1 << i))
				&& (available_mem_properties.memoryTypes[i].propertyFlags & memory_visibility) == memory_visibility)
			{
				return i;
			}
		}
		return static_cast<uint32_t>(-1);
	}

	class Core;

	// Buffer: Class for managing Vulkan buffer resources
	// - Represents a GPU buffer and its associated memory allocation
	// - Handles memory allocation, mapping, and resource cleanup
	class Buffer
	{
	public:
		// GetHandle: Returns the native Vulkan buffer handle
		vk::Buffer get_handle();

		// GetMemory: Returns the device memory handle for the buffer
		vk::DeviceMemory get_memory();

		// Map: Maps the buffer memory to CPU-accessible memory
		// Returns: Pointer to the mapped memory
		void* map();

		// Unmap: Unmaps the buffer memory from CPU access
		void unmap();


		// Constructor: Creates a new buffer with specified properties
		// Parameters:
		// - physicalDevice: Physical device for memory allocation
		// - logicalDevice: Logical device for buffer operations
		// - size: Size of the buffer in bytes
		// - usageFlags: Buffer usage flags (e.g. vertex buffer, uniform buffer)
		// - memoryVisibility: Memory property flags (e.g. host visible, device local)
		Buffer(vk::PhysicalDevice physical_device, vk::Device logical_device, vk::DeviceSize size,
		       vk::BufferUsageFlags usage_flags, vk::MemoryPropertyFlags memory_visibility);

	private:
		vk::UniqueBuffer buffer_handle_; // Native Vulkan buffer handle
		vk::UniqueDeviceMemory buffer_memory_; // Device memory allocation for this buffer
		vk::Device logical_device_; // Logical device for buffer operations
		vk::DeviceSize size_; // Size of the buffer in bytes
		friend class Core;
	};
}
