#include "StagedResources.h"

#include "Buffer.h"
#include "Core.h"
#include "PresentQueue.h"


namespace lz
{
	StagedBuffer::StagedBuffer(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice, vk::DeviceSize size,
		vk::BufferUsageFlags bufferUsage)
	{
		this->size = size;
		stagingBuffer = std::unique_ptr<lz::Buffer>(new lz::Buffer(physicalDevice, logicalDevice, size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
		deviceLocalBuffer = std::unique_ptr<lz::Buffer>(new lz::Buffer(physicalDevice, logicalDevice, size, bufferUsage | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal));
	}

	void* StagedBuffer::Map()
	{
		return stagingBuffer->Map();
	}

	void StagedBuffer::Unmap(vk::CommandBuffer commandBuffer)
	{
		stagingBuffer->Unmap();

		auto copyRegion = vk::BufferCopy()
		                  .setSrcOffset(0)
		                  .setDstOffset(0)
		                  .setSize(size);
		commandBuffer.copyBuffer(stagingBuffer->get_handle(), deviceLocalBuffer->get_handle(), { copyRegion });
	}

	vk::Buffer StagedBuffer::GetBuffer()
	{
		return deviceLocalBuffer->get_handle();
	}

	void LoadBufferData(lz::Core* core, void* bufferData, size_t bufferSize, lz::Buffer* dstBuffer)
	{
		auto stagingBuffer = std::unique_ptr<lz::Buffer>(new lz::Buffer(core->GetPhysicalDevice(), core->GetLogicalDevice(), bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));

		void* bufferMappedData = stagingBuffer->Map();
		memcpy(bufferMappedData, bufferData, bufferSize);
		stagingBuffer->Unmap();

		auto copyRegion = vk::BufferCopy()
		                  .setSrcOffset(0)
		                  .setDstOffset(0)
		                  .setSize(bufferSize);

		lz::ExecuteOnceQueue transferQueue(core);
		auto transferCommandBuffer = transferQueue.BeginCommandBuffer();
		{
			transferCommandBuffer.copyBuffer(stagingBuffer->get_handle(), dstBuffer->get_handle(), { copyRegion });
		}
		transferQueue.EndCommandBuffer();
	}
}
