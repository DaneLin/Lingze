#pragma once

#include <map>

#include "Config.h"
#include "ShaderProgram.h"

namespace lz
{
struct DescriptorSetBindings
{
	std::vector<UniformBufferBinding> uniform_buffer_bindings;
	std::vector<ImageSamplerBinding>  image_sampler_bindings;
	std::vector<StorageBufferBinding> storage_buffer_bindings;
	std::vector<StorageImageBinding>  storage_image_bindings;

	DescriptorSetBindings &set_uniform_buffer_bindings(const std::vector<UniformBufferBinding> &uniform_buffer_bindings);
	DescriptorSetBindings &set_image_sampler_bindings(const std::vector<ImageSamplerBinding> &image_sampler_bindings);
	DescriptorSetBindings &set_storage_buffer_bindings(const std::vector<StorageBufferBinding> &storage_buffer_bindings);
	DescriptorSetBindings &set_storage_image_bindings(const std::vector<StorageImageBinding> &storage_image_bindings);
};

class DescriptorSetCache
{
  public:
	explicit DescriptorSetCache(vk::Device logical_device);

	vk::DescriptorSetLayout get_descriptor_set_layout(const lz::DescriptorSetLayoutKey &descriptor_set_layout_key);

	vk::DescriptorSet get_descriptor_set(
	    const lz::DescriptorSetLayoutKey        &set_layout_key,
	    const std::vector<UniformBufferBinding> &uniform_buffer_bindings,
	    const std::vector<StorageBufferBinding> &storage_buffer_bindings,
	    const std::vector<ImageSamplerBinding>  &image_sampler_bindings);

	vk::DescriptorSet get_descriptor_set(const lz::DescriptorSetLayoutKey &set_layout_key, const lz::DescriptorSetBindings &set_bindings);

	void clear();

  private:
	struct DescriptorSetKey
	{
		vk::DescriptorSetLayout   layout;
		lz::DescriptorSetBindings bindings;
		bool                      operator<(const DescriptorSetKey &other) const;
	};

	std::map<lz::DescriptorSetLayoutKey, vk::UniqueDescriptorSetLayout> descriptor_set_layout_cache_;
	vk::UniqueDescriptorPool                                            descriptor_pool_;
	std::map<DescriptorSetKey, vk::UniqueDescriptorSet>                 descriptor_set_cache_;
	vk::Device                                                          logical_device_;
};
}        // namespace lz
