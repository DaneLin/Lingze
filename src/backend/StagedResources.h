#pragma once

#include "Buffer.h"
#include "Config.h"

namespace lz
{
class StagedBuffer
{
  public:
	StagedBuffer(vk::PhysicalDevice physical_device, vk::Device logical_device, vk::DeviceSize size, vk::BufferUsageFlags buffer_usage);

	void *map();

	void unmap(vk::CommandBuffer command_buffer);

  void* get_mapped_data();

	lz::Buffer &get_buffer();

  private:
	std::unique_ptr<lz::Buffer> staging_buffer_;
	std::unique_ptr<lz::Buffer> device_local_buffer_;
	vk::DeviceSize              size_;
	void*                       mapped_data_;
};

static void load_buffer_data(lz::Core *core, void *buffer_data, size_t buffer_size, lz::Buffer *dst_buffer);

// class StagedImage
// {
//   public:
// 	StagedImage(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice, glm::uvec2 size, uint32_t mipsCount, uint32_t arrayLayersCount)
// 	{
// 		this->core       = core;
// 		this->memorySize = size.x * size.y * 4;        // 4bpp
// 		this->imageSize  = size;

// 		stagingBuffer    = std::unique_ptr<lz::Buffer>(new lz::Buffer(physicalDevice, logicalDevice, memorySize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
// 		deviceLocalImage = std::unique_ptr<lz::Image>(new lz::Image(physicalDevice, logicalDevice, vk::ImageType::e2D, glm::uvec3(size.x, size.y, 1), mipsCount, arrayLayersCount, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled));
// 	}
// 	void *map()
// 	{
// 		return stagingBuffer->map();
// 	}
// 	void unmap(vk::CommandBuffer commandBuffer)
// 	{
// 		stagingBuffer->unmap();

// 		auto imageSubresource = vk::ImageSubresourceLayers()
// 		                            .setAspectMask(vk::ImageAspectFlagBits::eColor)
// 		                            .setMipLevel(0)
// 		                            .setBaseArrayLayer(0)
// 		                            .setLayerCount(1);

// 		auto copyRegion = vk::BufferImageCopy()
// 		                      .setBufferOffset(0)
// 		                      .setBufferRowLength(0)
// 		                      .setBufferImageHeight(0)
// 		                      .setImageSubresource(imageSubresource)
// 		                      .setImageOffset(vk::Offset3D(0, 0, 0))
// 		                      .setImageExtent(vk::Extent3D(imageSize.x, imageSize.y, 1));
    
// 		core->transition_image_layout(commandBuffer, deviceLocalImage->get_handle(), vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
// 		commandBuffer.copyBufferToImage(stagingBuffer->get_handle(), deviceLocalImage->get_handle(), vk::ImageLayout::eTransferDstOptimal, {copyRegion});
// 		core->transition_image_layout(commandBuffer, deviceLocalImage->get_handle(), vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
// 	}
// 	vk::Image get_image()
// 	{
// 		return deviceLocalImage->get_handle();
// 	}

//   private:
// 	std::unique_ptr<lz::Buffer> stagingBuffer;
// 	std::unique_ptr<lz::Image>  deviceLocalImage;
// 	glm::uvec2                  imageSize;
// 	vk::DeviceSize              memorySize;
// 	lz::Core                   *core;
// };
}        // namespace lz
