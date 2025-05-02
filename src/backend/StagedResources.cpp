#include "StagedResources.h"

#include "Buffer.h"
#include "Core.h"
#include "PresentQueue.h"

namespace lz
{
StagedBuffer::StagedBuffer(vk::PhysicalDevice physical_device, vk::Device logical_device, vk::DeviceSize size,
                           vk::BufferUsageFlags buffer_usage)
{
	this->size_          = size;
	staging_buffer_      = std::make_unique<lz::Buffer>(physical_device, logical_device, size,
	                                                    vk::BufferUsageFlagBits::eTransferSrc,
	                                                    vk::MemoryPropertyFlagBits::eHostVisible |
	                                                        vk::MemoryPropertyFlagBits::eHostCoherent);
	device_local_buffer_ = std::make_unique<lz::Buffer>(physical_device, logical_device, size,
	                                                    buffer_usage | vk::BufferUsageFlagBits::eTransferDst,
	                                                    vk::MemoryPropertyFlagBits::eDeviceLocal);
}

void *StagedBuffer::map()
{
	return staging_buffer_->map();
}

void StagedBuffer::unmap(vk::CommandBuffer command_buffer)
{
	staging_buffer_->unmap();

	auto copyRegion = vk::BufferCopy()
	                      .setSrcOffset(0)
	                      .setDstOffset(0)
	                      .setSize(size_);
	command_buffer.copyBuffer(staging_buffer_->get_handle(), device_local_buffer_->get_handle(), {copyRegion});
}

lz::Buffer &StagedBuffer::get_buffer()
{
	return *device_local_buffer_;
}

void load_buffer_data(lz::Core *core, void *buffer_data, size_t buffer_size, lz::Buffer *dst_buffer)
{
	auto staging_buffer = std::make_unique<lz::Buffer>(core->get_physical_device(), core->get_logical_device(),
	                                                   buffer_size, vk::BufferUsageFlagBits::eTransferSrc,
	                                                   vk::MemoryPropertyFlagBits::eHostVisible |
	                                                       vk::MemoryPropertyFlagBits::eHostCoherent);

	void *buffer_mapped_data = staging_buffer->map();
	memcpy(buffer_mapped_data, buffer_data, buffer_size);
	staging_buffer->unmap();

	auto copy_region = vk::BufferCopy()
	                       .setSrcOffset(0)
	                       .setDstOffset(0)
	                       .setSize(buffer_size);

	lz::ExecuteOnceQueue transfer_queue(core);
	const auto           transfer_command_buffer = transfer_queue.begin_command_buffer();
	{
		transfer_command_buffer.copyBuffer(staging_buffer->get_handle(), dst_buffer->get_handle(), {copy_region});
	}
	transfer_queue.end_command_buffer();
}
}        // namespace lz
