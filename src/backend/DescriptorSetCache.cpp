#include "DescriptorSetCache.h"

#include "ImageView.h"
#include "Sampler.h"
#include "ShaderProgram.h"

#include "Config.h"
#include "backend/EngineConfig.h"

namespace lz
{
DescriptorSetBindings &DescriptorSetBindings::set_uniform_buffer_bindings(
    const std::vector<UniformBufferBinding> &uniform_buffer_bindings)
{
	this->uniform_buffer_bindings = uniform_buffer_bindings;
	return *this;
}

DescriptorSetBindings &DescriptorSetBindings::set_image_sampler_bindings(
    const std::vector<ImageSamplerBinding> &image_sampler_bindings)
{
	this->image_sampler_bindings = image_sampler_bindings;
	return *this;
}

DescriptorSetBindings &DescriptorSetBindings::set_storage_buffer_bindings(
    const std::vector<StorageBufferBinding> &storage_buffer_bindings)
{
	this->storage_buffer_bindings = storage_buffer_bindings;
	return *this;
}

DescriptorSetBindings &DescriptorSetBindings::set_storage_image_bindings(
    const std::vector<StorageImageBinding> &storage_image_bindings)
{
	this->storage_image_bindings = storage_image_bindings;
	return *this;
}

DescriptorSetCache::DescriptorSetCache(const vk::Device logical_device, bool bindless_supported) :
    logical_device_(logical_device)
{
	std::vector<vk::DescriptorPoolSize> pool_sizes;

	constexpr auto uniform_pool_size = vk::DescriptorPoolSize()
	                                       .setDescriptorCount(COMMON_RESOURCE_COUNT)
	                                       .setType(vk::DescriptorType::eUniformBufferDynamic);
	pool_sizes.push_back(uniform_pool_size);

	constexpr auto image_sampler_pool_size = vk::DescriptorPoolSize()
	                                             .setDescriptorCount(COMMON_RESOURCE_COUNT)
	                                             .setType(vk::DescriptorType::eCombinedImageSampler);
	pool_sizes.push_back(image_sampler_pool_size);

	constexpr auto storage_pool_size = vk::DescriptorPoolSize()
	                                       .setDescriptorCount(COMMON_RESOURCE_COUNT)
	                                       .setType(vk::DescriptorType::eStorageBuffer);
	pool_sizes.push_back(storage_pool_size);

	constexpr auto storage_image_pool_size = vk::DescriptorPoolSize()
	                                             .setDescriptorCount(COMMON_RESOURCE_COUNT)
	                                             .setType(vk::DescriptorType::eStorageImage);
	pool_sizes.push_back(storage_image_pool_size);

	const auto pool_create_info = vk::DescriptorPoolCreateInfo()
	                                  .setMaxSets(COMMON_RESOURCE_COUNT)        // Allocate enough sets for all resource types
	                                  .setPoolSizeCount(uint32_t(pool_sizes.size()))
	                                  .setFlags(bindless_supported ? vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet | vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind : vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
	                                  .setPPoolSizes(pool_sizes.data());

	descriptor_pool_ = logical_device.createDescriptorPoolUnique(pool_create_info);
}

static uint32_t check_for_bindless_resources(uint32_t set_id, uint32_t set_binding)
{
	uint32_t descriptor_count = 1;

	if (set_id == BINDLESS_SET_ID)
	{
		if (set_binding == BINDLESS_TEXTURE_BINDING)
		{
			descriptor_count = BINDLESS_RESOURCE_COUNT;
		}
	}

	return descriptor_count;
}

vk::DescriptorSetLayout DescriptorSetCache::get_descriptor_set_layout(
    const lz::DescriptorSetLayoutKey &descriptor_set_layout_key)
{
	auto &descriptor_set_layout = descriptor_set_layout_cache_[descriptor_set_layout_key];
	bool  has_bindless          = false;
	if (!descriptor_set_layout)
	{
		std::vector<vk::DescriptorSetLayoutBinding> layout_bindings;
		std::vector<vk::DescriptorBindingFlags>     binding_flags;

		std::vector<lz::DescriptorSetLayoutKey::UniformBufferId> uniform_buffer_ids;
		uniform_buffer_ids.resize(descriptor_set_layout_key.get_uniform_buffers_count());
		descriptor_set_layout_key.get_uniform_buffer_ids(uniform_buffer_ids.data());

		for (auto uniform_buffer_id : uniform_buffer_ids)
		{
			auto buffer_info           = descriptor_set_layout_key.get_uniform_buffer_info(uniform_buffer_id);
			auto descriptor_count      = check_for_bindless_resources(descriptor_set_layout_key.get_set_id(), buffer_info.shader_binding_index);
			auto buffer_layout_binding = vk::DescriptorSetLayoutBinding()
			                                 .setBinding(buffer_info.shader_binding_index)
			                                 .setDescriptorCount(descriptor_count)        // if this is an array of buffers
			                                 .setDescriptorType(vk::DescriptorType::eUniformBufferDynamic)
			                                 .setStageFlags(buffer_info.stage_flags);
			layout_bindings.push_back(buffer_layout_binding);

			// Add binding flags
			vk::DescriptorBindingFlags flags = {};
			if (descriptor_set_layout_key.get_set_id() == BINDLESS_SET_ID && buffer_info.shader_binding_index == BINDLESS_BUFFER_BINDING)
			{
				has_bindless = true;

				flags = vk::DescriptorBindingFlagBits::eUpdateAfterBind |
				        vk::DescriptorBindingFlagBits::ePartiallyBound;

				if (descriptor_count > 1)
				{
					flags |= vk::DescriptorBindingFlagBits::eVariableDescriptorCount;
				}
			}
			binding_flags.push_back(flags);
		}

		std::vector<lz::DescriptorSetLayoutKey::StorageBufferId> storage_buffer_ids;
		storage_buffer_ids.resize(descriptor_set_layout_key.get_storage_buffers_count());
		descriptor_set_layout_key.get_storage_buffer_ids(storage_buffer_ids.data());

		for (auto storage_buffer_id : storage_buffer_ids)
		{
			auto buffer_info           = descriptor_set_layout_key.get_storage_buffer_info(storage_buffer_id);
			auto descriptor_count      = check_for_bindless_resources(descriptor_set_layout_key.get_set_id(), buffer_info.shader_binding_index);
			auto buffer_layout_binding = vk::DescriptorSetLayoutBinding()
			                                 .setBinding(buffer_info.shader_binding_index)
			                                 .setDescriptorCount(descriptor_count)        // if this is an array of buffers
			                                 .setDescriptorType(vk::DescriptorType::eStorageBuffer)
			                                 .setStageFlags(buffer_info.stage_flags);
			layout_bindings.push_back(buffer_layout_binding);

			// Add binding flags
			vk::DescriptorBindingFlags flags = {};
			if (descriptor_set_layout_key.get_set_id() == BINDLESS_SET_ID && buffer_info.shader_binding_index == BINDLESS_STORAGE_BUFFER_BINDING)
			{
				has_bindless = true;

				flags = vk::DescriptorBindingFlagBits::eUpdateAfterBind | vk::DescriptorBindingFlagBits::ePartiallyBound;

				if (descriptor_count > 1)
				{
					flags |= vk::DescriptorBindingFlagBits::eVariableDescriptorCount;
				}
			}
			binding_flags.push_back(flags);
		}

		std::vector<lz::DescriptorSetLayoutKey::ImageSamplerId> image_sampler_ids;
		image_sampler_ids.resize(descriptor_set_layout_key.get_image_samplers_count());
		descriptor_set_layout_key.get_image_sampler_ids(image_sampler_ids.data());

		for (auto image_sampler_id : image_sampler_ids)
		{
			auto image_sampler_info           = descriptor_set_layout_key.get_image_sampler_info(image_sampler_id);
			auto descriptor_count             = check_for_bindless_resources(descriptor_set_layout_key.get_set_id(), image_sampler_info.shader_binding_index);
			auto image_sampler_layout_binding = vk::DescriptorSetLayoutBinding()
			                                        .setBinding(image_sampler_info.shader_binding_index)
			                                        .setDescriptorCount(descriptor_count)
			                                        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			                                        .setStageFlags(image_sampler_info.stage_flags);
			layout_bindings.push_back(image_sampler_layout_binding);

			// Add binding flags
			vk::DescriptorBindingFlags flags = {};
			if (descriptor_set_layout_key.get_set_id() == BINDLESS_SET_ID && image_sampler_info.shader_binding_index == BINDLESS_TEXTURE_BINDING)
			{
				has_bindless = true;

				flags = vk::DescriptorBindingFlagBits::eUpdateAfterBind |
				        vk::DescriptorBindingFlagBits::ePartiallyBound;

				if (descriptor_count > 1)
				{
					flags |= vk::DescriptorBindingFlagBits::eVariableDescriptorCount;
				}
			}
			binding_flags.push_back(flags);
		}

		std::vector<lz::DescriptorSetLayoutKey::StorageImageId> storage_image_ids;
		storage_image_ids.resize(descriptor_set_layout_key.get_storage_images_count());
		descriptor_set_layout_key.get_storage_image_ids(storage_image_ids.data());

		for (auto storage_image_id : storage_image_ids)
		{
			auto image_info           = descriptor_set_layout_key.get_storage_image_info(storage_image_id);
			auto descriptor_count     = check_for_bindless_resources(descriptor_set_layout_key.get_set_id(), image_info.shader_binding_index);
			auto image_layout_binding = vk::DescriptorSetLayoutBinding()
			                                .setBinding(image_info.shader_binding_index)
			                                .setDescriptorCount(descriptor_count)        // if this is an array of buffers
			                                .setDescriptorType(vk::DescriptorType::eStorageImage)
			                                .setStageFlags(image_info.stage_flags);
			layout_bindings.push_back(image_layout_binding);

			// Add binding flags
			vk::DescriptorBindingFlags flags = {};
			if (descriptor_set_layout_key.get_set_id() == BINDLESS_SET_ID && image_info.shader_binding_index == BINDLESS_STORAGE_IMAGE_BINDING)
			{
				has_bindless = true;

				flags = vk::DescriptorBindingFlagBits::eUpdateAfterBind |
				        vk::DescriptorBindingFlagBits::ePartiallyBound;

				if (descriptor_count > 1)
				{
					flags |= vk::DescriptorBindingFlagBits::eVariableDescriptorCount;
				}
			}
			binding_flags.push_back(flags);
		}

		auto descriptor_layout_info = vk::DescriptorSetLayoutCreateInfo()
		                                  .setBindingCount(uint32_t(layout_bindings.size()))
		                                  .setPBindings(layout_bindings.data());

		// If this is a bindless set or has bindless resources, set appropriate flags
		vk::DescriptorSetLayoutBindingFlagsCreateInfo binding_flags_info;
		if (has_bindless && descriptor_set_layout_key.get_set_id() == BINDLESS_SET_ID && !binding_flags.empty())
		{
			binding_flags_info.setBindingCount(static_cast<uint32_t>(binding_flags.size()));
			binding_flags_info.setPBindingFlags(binding_flags.data());
			descriptor_layout_info.setFlags(vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool);
			descriptor_layout_info.setPNext(&binding_flags_info);
		}

		descriptor_set_layout = logical_device_.createDescriptorSetLayoutUnique(descriptor_layout_info);
	}
	return descriptor_set_layout.get();
}

vk::DescriptorSet DescriptorSetCache::get_descriptor_set(const lz::DescriptorSetLayoutKey &set_layout_key, const std::vector<UniformBufferBinding> &uniform_buffer_bindings, const std::vector<StorageBufferBinding> &storage_buffer_bindings, const std::vector<StorageImageBinding> &storage_image_bindings, const std::vector<ImageSamplerBinding> &image_sampler_bindings)
{
	const auto set_bindings = lz::DescriptorSetBindings()
	                              .set_uniform_buffer_bindings(uniform_buffer_bindings)
	                              .set_storage_buffer_bindings(storage_buffer_bindings)
	                              .set_storage_image_bindings(storage_image_bindings)
	                              .set_image_sampler_bindings(image_sampler_bindings);
	return get_descriptor_set(set_layout_key, set_bindings);
}


vk::DescriptorSet DescriptorSetCache::get_descriptor_set(const lz::DescriptorSetLayoutKey &set_layout_key,
                                                         const lz::DescriptorSetBindings  &set_bindings)
{
	DescriptorSetKey key;
	key.bindings = set_bindings;
	key.layout   = get_descriptor_set_layout(set_layout_key);

	auto &descriptor_set = descriptor_set_cache_[key];
	if (!descriptor_set)
	{
		auto set_alloc_info = vk::DescriptorSetAllocateInfo()
		                          .setDescriptorPool(this->descriptor_pool_.get())
		                          .setDescriptorSetCount(1)
		                          .setPSetLayouts(&key.layout);

		// Check if we need to handle variable descriptor counts for bindless sets
		vk::DescriptorSetVariableDescriptorCountAllocateInfo variable_count_info;
		uint32_t                                             max_binding_count = 0;

		if (set_layout_key.get_set_id() == BINDLESS_SET_ID)
		{
			// Find max descriptor count for this set's bindings
			std::vector<uint32_t> descriptor_counts;

			// Check uniform buffers
			std::vector<lz::DescriptorSetLayoutKey::UniformBufferId> uniform_buffer_ids;
			uniform_buffer_ids.resize(set_layout_key.get_uniform_buffers_count());
			set_layout_key.get_uniform_buffer_ids(uniform_buffer_ids.data());

			for (auto uniform_buffer_id : uniform_buffer_ids)
			{
				auto buffer_info = set_layout_key.get_uniform_buffer_info(uniform_buffer_id);
				auto count       = check_for_bindless_resources(set_layout_key.get_set_id(), buffer_info.shader_binding_index);
				if (count > max_binding_count)
					max_binding_count = count;
			}

			// Check storage buffers
			std::vector<lz::DescriptorSetLayoutKey::StorageBufferId> storage_buffer_ids;
			storage_buffer_ids.resize(set_layout_key.get_storage_buffers_count());
			set_layout_key.get_storage_buffer_ids(storage_buffer_ids.data());

			for (auto storage_buffer_id : storage_buffer_ids)
			{
				auto buffer_info = set_layout_key.get_storage_buffer_info(storage_buffer_id);
				auto count       = check_for_bindless_resources(set_layout_key.get_set_id(), buffer_info.shader_binding_index);
				if (count > max_binding_count)
					max_binding_count = count;
			}

			// Check image samplers
			std::vector<lz::DescriptorSetLayoutKey::ImageSamplerId> image_sampler_ids;
			image_sampler_ids.resize(set_layout_key.get_image_samplers_count());
			set_layout_key.get_image_sampler_ids(image_sampler_ids.data());

			for (auto image_sampler_id : image_sampler_ids)
			{
				auto image_info = set_layout_key.get_image_sampler_info(image_sampler_id);
				auto count      = check_for_bindless_resources(set_layout_key.get_set_id(), image_info.shader_binding_index);
				if (count > max_binding_count)
					max_binding_count = count;
			}

			// Check storage images
			std::vector<lz::DescriptorSetLayoutKey::StorageImageId> storage_image_ids;
			storage_image_ids.resize(set_layout_key.get_storage_images_count());
			set_layout_key.get_storage_image_ids(storage_image_ids.data());

			for (auto storage_image_id : storage_image_ids)
			{
				auto image_info = set_layout_key.get_storage_image_info(storage_image_id);
				auto count      = check_for_bindless_resources(set_layout_key.get_set_id(), image_info.shader_binding_index);
				if (count > max_binding_count)
					max_binding_count = count;
			}

			if (max_binding_count > 1)
			{
				descriptor_counts.push_back(max_binding_count);
				variable_count_info.setDescriptorSetCount(1)
				    .setPDescriptorCounts(descriptor_counts.data());
				set_alloc_info.setPNext(&variable_count_info);
			}
		}

		descriptor_set = std::move(logical_device_.allocateDescriptorSetsUnique(set_alloc_info)[0]);

		std::vector<vk::WriteDescriptorSet> set_writes;

		assert(set_layout_key.get_uniform_buffers_count() == set_bindings.uniform_buffer_bindings.size());
		std::vector<vk::DescriptorBufferInfo> uniform_buffer_infos(set_bindings.uniform_buffer_bindings.size());
		// cannot be kept in local variable
		for (size_t uniform_buffer_index = 0; uniform_buffer_index < set_bindings.uniform_buffer_bindings.size();
		     uniform_buffer_index++)
		{
			auto &uniform_binding = set_bindings.uniform_buffer_bindings[uniform_buffer_index];

			{
				auto uniform_buffer_id = set_layout_key.get_uniform_buffer_id(uniform_binding.shader_binding_id);
				assert(uniform_buffer_id.is_valid());
				auto uniform_buffer_data = set_layout_key.get_uniform_buffer_info(uniform_buffer_id);
				assert(uniform_buffer_data.size == uniform_binding.size);
			}

			uniform_buffer_infos[uniform_buffer_index] = vk::DescriptorBufferInfo()
			                                                 .setBuffer(uniform_binding.buffer->get_handle())
			                                                 .setOffset(uniform_binding.offset)
			                                                 .setRange(uniform_binding.size);

			auto set_write = vk::WriteDescriptorSet()
			                     .setDescriptorCount(1)
			                     .setDescriptorType(vk::DescriptorType::eUniformBufferDynamic)
			                     .setDstBinding(uniform_binding.shader_binding_id)
			                     .setDstSet(descriptor_set.get())
			                     .setPBufferInfo(&uniform_buffer_infos[uniform_buffer_index]);

			set_writes.push_back(set_write);
		}

		assert(set_bindings.image_sampler_bindings.size() == set_layout_key.get_image_samplers_count());
		std::vector<vk::DescriptorImageInfo> image_sampler_infos(set_bindings.image_sampler_bindings.size());
		for (size_t image_sampler_index = 0; image_sampler_index < set_bindings.image_sampler_bindings.size();
		     image_sampler_index++)
		{
			auto &image_sampler_binding = set_bindings.image_sampler_bindings[image_sampler_index];

			{
				auto image_sampler_id = set_layout_key.get_image_sampler_id(image_sampler_binding.shader_binding_id);
				assert(image_sampler_id.is_valid());
				auto image_sampler_data = set_layout_key.get_image_sampler_info(image_sampler_id);
			}

			image_sampler_infos[image_sampler_index] = vk::DescriptorImageInfo()
			                                               .setImageView(image_sampler_binding.image_view->get_handle())
			                                               .setSampler(image_sampler_binding.sampler->get_handle())
			                                               .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

			// auto imageInfo
			auto set_write = vk::WriteDescriptorSet()
			                     .setDescriptorCount(1)
			                     .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			                     .setDstBinding(image_sampler_binding.shader_binding_id)
			                     .setDstSet(descriptor_set.get())
			                     .setPImageInfo(&image_sampler_infos[image_sampler_index]);

			set_writes.push_back(set_write);
		}

		assert(set_bindings.storage_buffer_bindings.size() == set_layout_key.get_storage_buffers_count());

		std::vector<vk::DescriptorBufferInfo> storage_buffer_infos(set_bindings.storage_buffer_bindings.size());

		for (size_t storage_buffer_index = 0; storage_buffer_index < set_bindings.storage_buffer_bindings.size();
		     storage_buffer_index++)
		{
			auto &storage_binding = set_bindings.storage_buffer_bindings[storage_buffer_index];

			{
				auto storage_buffer_id = set_layout_key.get_storage_buffer_id(storage_binding.shader_binding_id);
				assert(storage_buffer_id.is_valid());
				auto storage_buffer_data = set_layout_key.get_storage_buffer_info(storage_buffer_id);
				// assert(storageBufferData.size == storageBinding.size);
			}

			storage_buffer_infos[storage_buffer_index] = vk::DescriptorBufferInfo()
			                                                 .setBuffer(storage_binding.buffer->get_handle())
			                                                 .setOffset(storage_binding.offset)
			                                                 .setRange(storage_binding.size);

			auto set_write = vk::WriteDescriptorSet()
			                     .setDescriptorCount(1)
			                     .setDescriptorType(vk::DescriptorType::eStorageBuffer)
			                     .setDstBinding(storage_binding.shader_binding_id)
			                     .setDstSet(descriptor_set.get())
			                     .setPBufferInfo(&storage_buffer_infos[storage_buffer_index]);

			set_writes.push_back(set_write);
		}

		assert(set_bindings.storage_image_bindings.size() == set_layout_key.get_storage_images_count());
		std::vector<vk::DescriptorImageInfo> storage_image_infos(set_bindings.storage_image_bindings.size());
		for (size_t storage_image_index = 0; storage_image_index < set_bindings.storage_image_bindings.size();
		     storage_image_index++)
		{
			auto &storage_binding = set_bindings.storage_image_bindings[storage_image_index];

			{
				auto storage_image_id = set_layout_key.get_storage_image_id(storage_binding.shader_binding_id);
				assert(storage_image_id.is_valid());
				auto storage_image_data = set_layout_key.get_storage_image_info(storage_image_id);
				// assert(storageImageData.format == storageBinding.format);
			}

			storage_image_infos[storage_image_index] = vk::DescriptorImageInfo()
			                                               .setImageView(storage_binding.image_view->get_handle())
			                                               .setImageLayout(vk::ImageLayout::eGeneral);

			if (set_bindings.image_sampler_bindings.size() > 0)
				storage_image_infos[storage_image_index].setSampler(
				    set_bindings.image_sampler_bindings[0].sampler->get_handle());

			auto set_write = vk::WriteDescriptorSet()
			                     .setDescriptorCount(1)
			                     .setDescriptorType(vk::DescriptorType::eStorageImage)
			                     .setDstBinding(storage_binding.shader_binding_id)
			                     .setDstSet(descriptor_set.get())
			                     .setPImageInfo(&storage_image_infos[storage_image_index]);

			set_writes.push_back(set_write);
		}
		logical_device_.updateDescriptorSets(set_writes, {});
	}
	return descriptor_set.get();
}

void DescriptorSetCache::clear()
{
	this->descriptor_set_cache_.clear();
	this->descriptor_set_layout_cache_.clear();
}

bool DescriptorSetCache::DescriptorSetKey::operator<(const DescriptorSetKey &other) const
{
	return std::tie(layout, bindings.uniform_buffer_bindings, bindings.storage_buffer_bindings, bindings.storage_image_bindings, bindings.image_sampler_bindings) < std::tie(other.layout, other.bindings.uniform_buffer_bindings, other.bindings.storage_buffer_bindings, other.bindings.storage_image_bindings, other.bindings.image_sampler_bindings);
}
}        // namespace lz
