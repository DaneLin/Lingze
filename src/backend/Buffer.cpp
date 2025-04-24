#include "Buffer.h"

namespace lz
{
	vk::Buffer Buffer::get_handle()
	{
		return buffer_handle_.get();
	}

	vk::DeviceMemory Buffer::get_memory()
	{
		return buffer_memory_.get();
	}

	void* Buffer::map()
	{
		return logical_device_.mapMemory(get_memory(), 0, size_);
	}

	void Buffer::unmap()
	{
		logical_device_.unmapMemory(get_memory());
	}

	Buffer::Buffer(const vk::PhysicalDevice physical_device, const vk::Device logical_device, const vk::DeviceSize size,
	               const vk::BufferUsageFlags usage_flags, const vk::MemoryPropertyFlags memory_visibility)
		: size_(size),
		  logical_device_(logical_device)
	{
		// Create the buffer resource
		const auto buffer_info = vk::BufferCreateInfo()
		                  .setSize(size)
		                  .setUsage(usage_flags)
		                  .setSharingMode(vk::SharingMode::eExclusive);
		buffer_handle_ = logical_device.createBufferUnique(buffer_info);

		// Get memory requirements for the buffer
		const vk::MemoryRequirements buffer_mem_requirements = logical_device.
			getBufferMemoryRequirements(buffer_handle_.get());

		// Allocate memory for the buffer
		const auto alloc_info = vk::MemoryAllocateInfo()
		                 .setAllocationSize(buffer_mem_requirements.size)
		                 .setMemoryTypeIndex(find_memory_type_index(physical_device,
		                                                            buffer_mem_requirements.memoryTypeBits,
		                                                            memory_visibility));

		buffer_memory_ = logical_device.allocateMemoryUnique(alloc_info);

		// Bind the buffer to the allocated memory
		logical_device.bindBufferMemory(buffer_handle_.get(), buffer_memory_.get(), 0);
	}
}
