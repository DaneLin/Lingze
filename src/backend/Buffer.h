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
	static uint32_t find_memory_type_index(vk::PhysicalDevice physicalDevice, uint32_t suitableIndices, vk::MemoryPropertyFlags memoryVisibility)
	{
		const vk::PhysicalDeviceMemoryProperties availableMemProperties = physicalDevice.getMemoryProperties();

		for (uint32_t i = 0; i < availableMemProperties.memoryTypeCount; ++i)
		{
			if ((suitableIndices & (1 << i)) && (availableMemProperties.memoryTypes[i].propertyFlags & memoryVisibility)
				== memoryVisibility)
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
		vk::DeviceMemory GetMemory();

		// Map: Maps the buffer memory to CPU-accessible memory
		// Returns: Pointer to the mapped memory
		void* Map();

		// Unmap: Unmaps the buffer memory from CPU access
		void Unmap();


		// Constructor: Creates a new buffer with specified properties
		// Parameters:
		// - physicalDevice: Physical device for memory allocation
		// - logicalDevice: Logical device for buffer operations
		// - size: Size of the buffer in bytes
		// - usageFlags: Buffer usage flags (e.g. vertex buffer, uniform buffer)
		// - memoryVisibility: Memory property flags (e.g. host visible, device local)
		Buffer(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice, vk::DeviceSize size,
			vk::BufferUsageFlags usageFlags, vk::MemoryPropertyFlags memoryVisibility);

	private:
		vk::UniqueBuffer bufferHandle;       // Native Vulkan buffer handle
		vk::UniqueDeviceMemory bufferMemory; // Device memory allocation for this buffer
		vk::Device logicalDevice;            // Logical device for buffer operations
		vk::DeviceSize size;                 // Size of the buffer in bytes
		friend class Core;
	};
}
