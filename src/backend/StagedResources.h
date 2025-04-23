#pragma once

#include "Buffer.h"
#include "LingzeVK.h"

namespace lz
{
    class StagedBuffer
    {
    public:
        StagedBuffer(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice, vk::DeviceSize size, vk::BufferUsageFlags bufferUsage);

        void* Map();

        void Unmap(vk::CommandBuffer commandBuffer);

        vk::Buffer GetBuffer();

    private:
        std::unique_ptr<lz::Buffer> stagingBuffer;
        std::unique_ptr<lz::Buffer> deviceLocalBuffer;
        vk::DeviceSize size;
    };

    static void LoadBufferData(lz::Core* core, void* bufferData, size_t bufferSize, lz::Buffer* dstBuffer);


    /*class StagedImage
    {
    public:
      StagedImage(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice, glm::uvec2 size, uint32_t mipsCount, uint32_t arrayLayersCount)
      {
        this->core = core;
        this->memorySize = size.x * size.y * 4; //4bpp
        this->imageSize = size;

        stagingBuffer = std::unique_ptr<lz::Buffer>(new lz::Buffer(physicalDevice, logicalDevice, memorySize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
        deviceLocalImage = std::unique_ptr<lz::Image>(new lz::Image(physicalDevice, logicalDevice, vk::ImageType::e2D, glm::uvec3(size.x, size.y, 1), mipsCount, arrayLayersCount, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled));
      }
      void *Map()
      {
        return stagingBuffer->Map();
      }
      void Unmap(vk::CommandBuffer commandBuffer)
      {
        stagingBuffer->Unmap();

        auto imageSubresource = vk::ImageSubresourceLayers()
          .setAspectMask(vk::ImageAspectFlagBits::eColor)
          .setMipLevel(0)
          .setBaseArrayLayer(0)
          .setLayerCount(1);

        auto copyRegion = vk::BufferImageCopy()
          .setBufferOffset(0)
          .setBufferRowLength(0)
          .setBufferImageHeight(0)
          .setImageSubresource(imageSubresource)
          .setImageOffset(vk::Offset3D(0, 0, 0))
          .setImageExtent(vk::Extent3D(imageSize.x, imageSize.y, 1));

        deviceLocalImage->GetData()->TransitionLayout(vk::ImageLayout::eTransferDstOptimal, commandBuffer);
        commandBuffer.copyBufferToImage(stagingBuffer->GetHandle(), deviceLocalImage->GetData()->GetHandle(), vk::ImageLayout::eTransferDstOptimal, { copyRegion });
        deviceLocalImage->GetData()->TransitionLayout(vk::ImageLayout::eShaderReadOnlyOptimal, commandBuffer);
      }
      vk::Image GetImage()
      {
        return deviceLocalImage->GetData()->GetHandle();
      }
    private:
      std::unique_ptr<lz::Buffer> stagingBuffer;
      std::unique_ptr<lz::Image> deviceLocalImage;
      glm::uvec2 imageSize;
      vk::DeviceSize memorySize;
      lz::Core *core;
    };*/
}
