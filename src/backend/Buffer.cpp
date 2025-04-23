#include "Buffer.h"

namespace lz
{
	vk::Buffer Buffer::get_handle()
	{
		return bufferHandle.get();
	}

	vk::DeviceMemory Buffer::GetMemory()
	{
		return bufferMemory.get();
	}

	void* Buffer::Map()
	{
		return logicalDevice.mapMemory(GetMemory(), 0, size);
	}

	void Buffer::Unmap()
	{
		logicalDevice.unmapMemory(GetMemory());
	}

	Buffer::Buffer(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice, vk::DeviceSize size,
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
		                 .setMemoryTypeIndex(find_memory_type_index(physicalDevice,
		                                                            bufferMemRequirements.memoryTypeBits,
		                                                            memoryVisibility));

		bufferMemory = logicalDevice.allocateMemoryUnique(allocInfo);

		// Bind the buffer to the allocated memory
		logicalDevice.bindBufferMemory(bufferHandle.get(), bufferMemory.get(), 0);
	}
}
