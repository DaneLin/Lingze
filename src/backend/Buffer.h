#pragma once
namespace lz
{
	// FindMemoryTypeIndex: Finds a suitable memory type index for a buffer allocation
	// Parameters:
	// - physicalDevice: Physical device to query memory properties from
	// - suitableIndices: Bit field of suitable memory type indices
	// - memoryVisibility: Required memory property flags
	// Returns: Index of a suitable memory type, or -1 if none found
	static uint32_t FindMemoryTypeIndex(vk::PhysicalDevice physicalDevice, uint32_t suitableIndices,
	                                    vk::MemoryPropertyFlags memoryVisibility)
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
		vk::Buffer GetHandle()
		{
			return bufferHandle.get();
		}

		// GetMemory: Returns the device memory handle for the buffer
		vk::DeviceMemory GetMemory()
		{
			return bufferMemory.get();
		}

		// Map: Maps the buffer memory to CPU-accessible memory
		// Returns: Pointer to the mapped memory
		void* Map()
		{
			return logicalDevice.mapMemory(GetMemory(), 0, size);
		}

		// Unmap: Unmaps the buffer memory from CPU access
		void Unmap()
		{
			logicalDevice.unmapMemory(GetMemory());
		}

		// Constructor: Creates a new buffer with specified properties
		// Parameters:
		// - physicalDevice: Physical device for memory allocation
		// - logicalDevice: Logical device for buffer operations
		// - size: Size of the buffer in bytes
		// - usageFlags: Buffer usage flags (e.g. vertex buffer, uniform buffer)
		// - memoryVisibility: Memory property flags (e.g. host visible, device local)
		Buffer(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice, vk::DeviceSize size,
		       vk::BufferUsageFlags usageFlags, vk::MemoryPropertyFlags memoryVisibility)
		{
			this->logicalDevice = logicalDevice;
			this->size = size;
			
			// Create the buffer resource
			auto bufferInfo = vk::BufferCreateInfo()
			                  .setSize(size)
			                  .setUsage(usageFlags)
			                  .setSharingMode(vk::SharingMode::eExclusive);
			bufferHandle = logicalDevice.createBufferUnique(bufferInfo);

			// Get memory requirements for the buffer
			vk::MemoryRequirements bufferMemRequirements = logicalDevice.
				getBufferMemoryRequirements(bufferHandle.get());

			// Allocate memory for the buffer
			auto allocInfo = vk::MemoryAllocateInfo()
			                 .setAllocationSize(bufferMemRequirements.size)
			                 .setMemoryTypeIndex(FindMemoryTypeIndex(physicalDevice,
			                                                         bufferMemRequirements.memoryTypeBits,
			                                                         memoryVisibility));

			bufferMemory = logicalDevice.allocateMemoryUnique(allocInfo);

			// Bind the buffer to the allocated memory
			logicalDevice.bindBufferMemory(bufferHandle.get(), bufferMemory.get(), 0);
		}

	private:
		vk::UniqueBuffer bufferHandle;       // Native Vulkan buffer handle
		vk::UniqueDeviceMemory bufferMemory; // Device memory allocation for this buffer
		vk::Device logicalDevice;            // Logical device for buffer operations
		vk::DeviceSize size;                 // Size of the buffer in bytes
		friend class Core;
	};
}
