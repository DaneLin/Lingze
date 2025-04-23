#pragma once

#include <map>

#include "LingzeVK.h"
#include "ShaderProgram.h"

namespace lz
{
	struct DescriptorSetBindings
	{
		std::vector<UniformBufferBinding> uniformBufferBindings;
		std::vector<ImageSamplerBinding> imageSamplerBindings;
		std::vector<StorageBufferBinding> storageBufferBindings;
		std::vector<StorageImageBinding> storageImageBindings;

        DescriptorSetBindings& SetUniformBufferBindings(std::vector<UniformBufferBinding> uniformBufferBindings);
		DescriptorSetBindings& SetImageSamplerBindings(std::vector<ImageSamplerBinding> imageSamplerBindings);
		DescriptorSetBindings& SetStorageBufferBindings(std::vector<StorageBufferBinding> storageBufferBindings);
		DescriptorSetBindings& SetStorageImageBindings(std::vector<StorageImageBinding> storageImageBindings);
	};

	

    class DescriptorSetCache
    {
    public:
        DescriptorSetCache(vk::Device logicalDevice);

        vk::DescriptorSetLayout GetDescriptorSetLayout(const lz::DescriptorSetLayoutKey& descriptorSetLayoutKey);

        vk::DescriptorSet GetDescriptorSet(
            lz::DescriptorSetLayoutKey setLayoutKey,
            const std::vector<UniformBufferBinding>& uniformBufferBindings,
            const std::vector<StorageBufferBinding>& storageBufferBindings,
            const std::vector<ImageSamplerBinding>& imageSamplerBindings);

        vk::DescriptorSet GetDescriptorSet(lz::DescriptorSetLayoutKey setLayoutKey, const lz::DescriptorSetBindings& setBindings);

        void Clear();

    private:
        struct DescriptorSetKey
        {
            vk::DescriptorSetLayout layout;
            lz::DescriptorSetBindings bindings;
            bool operator<(const DescriptorSetKey& other) const;
        };

        std::map<lz::DescriptorSetLayoutKey, vk::UniqueDescriptorSetLayout> descriptorSetLayoutCache;
        vk::UniqueDescriptorPool descriptorPool;
		std::map<DescriptorSetKey, vk::UniqueDescriptorSet> descriptorSetCache;
        vk::Device logicalDevice;
    };

    
}
