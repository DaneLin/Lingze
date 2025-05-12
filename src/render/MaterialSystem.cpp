#include "MaterialSystem.h"
#include <backend/Core.h>

#include "backend/Image.h"
#include "backend/ImageLoader.h"
#include "backend/ImageView.h"
#include "backend/Logging.h"

#include "backend/PresentQueue.h"

namespace lz
{
uint32_t MaterialSystem::allocate_texture_slot()
{
	uint32_t index;
	if (!free_texture_slots_.empty())
	{
		index = free_texture_slots_.back();
		free_texture_slots_.pop_back();
	}
	else
	{
		index = texture_count_++;
	}
	return index;
}
uint32_t MaterialSystem::allocate_material_slot()
{
	uint32_t index;
	if (!free_material_slots_.empty())
	{
		index = free_material_slots_.back();
		free_material_slots_.pop_back();
	}
	else
	{
		index = material_count_++;
	}
	return index;
}
void MaterialSystem::update_material_parameters(const std::shared_ptr<Material> &material)
{
	uint32_t material_index = get_material_index(material->name);
	if (material_index == UINT32_MAX)
	{
		LOGW("Material %s not found", material->name.c_str());
		return;
	}

	// submit update request
	UpdateRequest request;
	request.type          = UpdateRequest::UpdateType::eMaterialParametersUpdate;
	request.material_name = material->name;
	request_update(request);
}

MaterialSystem::MaterialSystem(Core *core) :
    core_(core)
{
	texture_images_.resize(k_max_bindless_resources);
	texture_views_.resize(k_max_bindless_resources);

	vk::SamplerCreateInfo sampler_create_info;
	sampler_create_info.setMagFilter(vk::Filter::eLinear)
	    .setMinFilter(vk::Filter::eLinear)
	    .setAddressModeU(vk::SamplerAddressMode::eRepeat)
	    .setAddressModeV(vk::SamplerAddressMode::eRepeat)
	    .setAddressModeW(vk::SamplerAddressMode::eRepeat)
	    .setAnisotropyEnable(VK_FALSE)
	    .setMaxAnisotropy(16.0f)
	    .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
	    .setUnnormalizedCoordinates(VK_FALSE)
	    .setCompareEnable(VK_FALSE)
	    .setCompareOp(vk::CompareOp::eAlways)
	    .setMipmapMode(vk::SamplerMipmapMode::eLinear)
	    .setMipLodBias(0.0f)
	    .setMinLod(0.0f)
	    .setMaxLod(16.0f);

	default_sampler_ = std::make_unique<Sampler>(core_->get_logical_device(), sampler_create_info);

	staging_buffer_ = std::make_unique<Buffer>(
	    core_->get_physical_device(),
	    core_->get_logical_device(),
	    sizeof(MaterialParameters) * k_max_bindless_resources,
	    vk::BufferUsageFlagBits::eTransferSrc,
	    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	staging_buffer_->map();

	material_parameters_buffer_ = std::make_unique<Buffer>(
	    core_->get_physical_device(),
	    core_->get_logical_device(),
	    sizeof(MaterialParameters) * k_max_bindless_resources,
	    vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
	    vk::MemoryPropertyFlagBits::eDeviceLocal);

	materials_.resize(k_max_bindless_resources);

	initialize();
}
MaterialSystem::~MaterialSystem()
{
	staging_buffer_->unmap();
}
void MaterialSystem::initialize()
{
	// creating descriptor pool
	std::vector<vk::DescriptorPoolSize> pool_sizes = {
	    {vk::DescriptorType::eCombinedImageSampler, k_max_bindless_resources},
	};

	vk::DescriptorPoolCreateInfo pool_create_info =
	    vk::DescriptorPoolCreateInfo()
	        .setPoolSizeCount(static_cast<uint32_t>(pool_sizes.size()))
	        .setPPoolSizes(pool_sizes.data())
	        .setMaxSets(k_max_bindless_resources)
	        .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet | vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind);

	try
	{
		bindless_descriptor_pool_ = core_->get_logical_device().createDescriptorPoolUnique(pool_create_info);
		DLOGI("Created bindless descriptor pool successfully");
	}
	catch (const std::exception &e)
	{
		LOGE("Failed to create bindless descriptor pool: %s", e.what());
		return;
	}

	// creating descriptor set layout

	std::vector<vk::DescriptorSetLayoutBinding> bindings(1);
	bindings[0].setBinding(k_bindless_texture_binding).setDescriptorType(vk::DescriptorType::eCombinedImageSampler).setDescriptorCount(k_max_bindless_resources) 
	    .setStageFlags(vk::ShaderStageFlagBits::eFragment);

	vk::DescriptorSetLayoutBindingFlagsCreateInfo binding_flags_info;
	std::vector<vk::DescriptorBindingFlags>       flags(bindings.size(),
	                                                    vk::DescriptorBindingFlagBits::ePartiallyBound |
	                                                        vk::DescriptorBindingFlagBits::eUpdateAfterBind |
	                                                        vk::DescriptorBindingFlagBits::eVariableDescriptorCount);
	binding_flags_info.setBindingCount(static_cast<uint32_t>(bindings.size()));
	binding_flags_info.setPBindingFlags(flags.data());

	vk::DescriptorSetLayoutCreateInfo layout_info;
	layout_info.setBindingCount(static_cast<uint32_t>(bindings.size()))
	    .setPBindings(bindings.data())
	    .setFlags(vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool)
	    .setPNext(&binding_flags_info);

	try
	{
		bindless_descriptor_set_layout_ = core_->get_logical_device().createDescriptorSetLayoutUnique(layout_info);
		DLOGI("Created bindless descriptor set layout successfully");
	}
	catch (const std::exception &e)
	{
		LOGE("Failed to create bindless descriptor set layout: {}", e.what());
		return;
	}

	// creating descriptor set
	vk::DescriptorSetAllocateInfo alloc_info;
	alloc_info.setDescriptorPool(bindless_descriptor_pool_.get())
	    .setDescriptorSetCount(1)
	    .setPSetLayouts(&bindless_descriptor_set_layout_.get());

	vk::DescriptorSetVariableDescriptorCountAllocateInfo count_allocate_info;
	count_allocate_info.setDescriptorSetCount(1)
	    .setPDescriptorCounts(&k_max_bindless_resources);

	alloc_info.pNext = &count_allocate_info;

	bindless_descriptor_set_ = std::move(core_->get_logical_device().allocateDescriptorSetsUnique(alloc_info)[0]);
	DLOGI("Successfully allocated bindless descriptor set");
}
uint32_t MaterialSystem::register_material(const std::shared_ptr<Material> &material)
{
	DLOGI("Registering material : {}", material->name.c_str());

	// check if material already exists
	auto it = material_name_to_index_.find(material->name);
	if (it != material_name_to_index_.end())
	{
		return it->second;
	}

	// allocate material slot
	uint32_t material_index                 = allocate_material_slot();
	materials_[material_index]              = material;
	material_name_to_index_[material->name] = material_index;

	// update
	upload_texture(material->diffuse_texture);
	upload_texture(material->normal_texture);
	upload_texture(material->metallic_roughness_texture);
	upload_texture(material->emissive_texture);
	upload_texture(material->occlusion_texture);
	update_material_parameters(material);

	return material_index;
}
uint32_t MaterialSystem::get_material_index(const std::string &material_name) const
{
	auto it = material_name_to_index_.find(material_name);
	if (it != material_name_to_index_.end())
	{
		return it->second;
	}
	return UINT32_MAX;
}
uint32_t MaterialSystem::upload_texture(const std::shared_ptr<Texture> &texture)
{
	if (!texture)
	{
		return UINT32_MAX;
	}

	// Check for missing data
	if (texture->data.empty() || texture->width <= 0 || texture->height <= 0 || texture->channels <= 0)
	{
		DLOGW("Texture has invalid dimensions or empty data: {}", texture->name);
		return UINT32_MAX;
	}

	// check if texture already exists
	if (!texture->name.empty())
	{
		auto it = texture_name_to_index_.find(texture->name);
		if (it != texture_name_to_index_.end())
		{
			return it->second;
		}
	}
	else
	{
		DLOGW("Texture has empty name, skipping upload");
		return UINT32_MAX;
	}

	uint32_t texture_index = allocate_texture_slot();

	if (texture_index >= k_max_bindless_resources)
	{
		DLOGW("Exceeded maximum bindless resources limit");
		return UINT32_MAX;
	}

	texture_name_to_index_[texture->name] = texture_index;

	// create ImageTexelData
	ImageTexelData texel_data;
	texel_data.texels     = texture->data;
	texel_data.base_size  = glm::uvec3(static_cast<uint32_t>(texture->width), static_cast<uint32_t>(texture->height), 1);
	texel_data.texel_size = texture->width * texture->height * texture->channels;

	vk::Format format;
	switch (texture->channels)
	{
		case 1:
			format = vk::Format::eR8Unorm;
			break;
		case 2:
			format = vk::Format::eR8G8Unorm;
			break;
		case 3:
			format = vk::Format::eR8G8B8Unorm;
			break;
		case 4:
		default:
			format = vk::Format::eR8G8B8A8Unorm;
			break;
	}

	try
	{
		auto image_create_info = Image::create_info_2d(
		    glm::uvec2(texture->width, texture->height),
		    1,
		    1,
		    format,
		    vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);

		texture_images_[texture_index] = std::make_unique<Image>(
		    core_->get_physical_device(),
		    core_->get_logical_device(),
		    image_create_info,
		    vk::MemoryPropertyFlagBits::eDeviceLocal);

		texture_views_[texture_index] = std::make_unique<ImageView>(
		    core_->get_logical_device(),
		    texture_images_[texture_index]->get_image_data(),
		    0, 1, 0, 1);

		// submit texture data when processing update queue
		UpdateRequest request;
		request.type    = UpdateRequest::UpdateType::eTextureUpload;
		request.texture = texture;

		// add to update queue
		request_update(request);
	}
	catch (const std::exception &e)
	{
		// Remove from the index map
		texture_name_to_index_.erase(texture->name);
		// Add the index back to the free list
		free_texture_slots_.push_back(texture_index);
		return UINT32_MAX;
	}

	return texture_index;
}
void MaterialSystem::request_update(const UpdateRequest &request)
{
	pending_updates_.push(request);
}
void MaterialSystem::process_pending_updates()
{
	std::vector<UpdateRequest> updates;

	{
		while (!pending_updates_.empty())
		{
			updates.push_back(pending_updates_.front());
			pending_updates_.pop();
		}
	}

	if (updates.empty())
	{
		return;
	}

	// Local vectors to store image infos and descriptor writes for this batch
	std::vector<vk::DescriptorImageInfo> image_infos;
	std::vector<vk::WriteDescriptorSet>  descriptor_writes;
	image_infos.reserve(updates.size());
	descriptor_writes.reserve(updates.size());

	for (const auto &request : updates)
	{
		if (request.type == UpdateRequest::UpdateType::eTextureUpload)
		{
			DLOGI("Uploading texture: {}", request.texture->name);
			uint32_t texture_index;
			if (!request.texture->name.empty())
			{
				auto it = texture_name_to_index_.find(request.texture->name);
				if (it == texture_name_to_index_.end())
				{
					LOGW("Texture {} not found in map", request.texture->name.c_str());
					continue;
				}
				texture_index = it->second;
			}
			else
			{
				continue;
			}

			// Validate texture_index is within range
			if (texture_index >= texture_images_.size() || texture_images_[texture_index] == nullptr || texture_views_[texture_index] == nullptr)
			{
				LOGW("Invalid texture index or null texture image/view at index {}", texture_index);
				continue;
			}

			ImageTexelData texel_data;
			texel_data.base_size    = glm::uvec3(request.texture->width, request.texture->height, 1);
			texel_data.layers_count = 1;
			texel_data.format       = texture_images_[texture_index]->get_image_data()->get_format();
			texel_data.texel_size   = request.texture->width * request.texture->height * request.texture->channels;
			texel_data.mips.resize(1);
			texel_data.mips[0].size = texel_data.base_size;
			texel_data.mips[0].layers.resize(1);
			texel_data.mips[0].layers[0].offset = 0;

			texel_data.texels = request.texture->data;

			load_texel_data(
			    core_,
			    &texel_data,
			    texture_images_[texture_index]->get_image_data(),
			    ImageUsageTypes::eGraphicsShaderRead);

			// Store the image info for this texture
			image_infos.emplace_back(
			    vk::DescriptorImageInfo()
			        .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
			        .setImageView(texture_views_[texture_index]->get_handle())
			        .setSampler(default_sampler_->get_handle()));

			// Create a descriptor write using the last added image info
			descriptor_writes.emplace_back(
			    vk::WriteDescriptorSet()
			        .setDstSet(bindless_descriptor_set_.get())
			        .setDstBinding(k_bindless_texture_binding)
			        .setDstArrayElement(texture_index)
			        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			        .setDescriptorCount(1)
			        .setPImageInfo(&image_infos.back()));
		}
		else if (request.type == UpdateRequest::UpdateType::eMaterialParametersUpdate)
		{
			uint32_t material_index = get_material_index(request.material_name);
			if (material_index == UINT32_MAX || material_index >= materials_.size())
			{
				LOGW("Invalid material index for {}", request.material_name.c_str());
				continue;
			}

			MaterialParameters *mapped_params = static_cast<MaterialParameters *>(staging_buffer_->get_mapped_data());

			MaterialParameters &params              = mapped_params[material_index];
			params.base_color_factor                = materials_[material_index]->base_color_factor;
			params.metallic_factor                  = materials_[material_index]->metallic_factor;
			params.roughness_factor                 = materials_[material_index]->roughness_factor;
			params.emissive_factor                  = materials_[material_index]->emissive_factor;
			params.diffuse_texture_index            = get_texture_index(materials_[material_index]->diffuse_texture);
			params.normal_texture_index             = get_texture_index(materials_[material_index]->normal_texture);
			params.metallic_roughness_texture_index = get_texture_index(materials_[material_index]->metallic_roughness_texture);
			params.emissive_texture_index           = get_texture_index(materials_[material_index]->emissive_texture);
			params.occlusion_texture_index          = get_texture_index(materials_[material_index]->occlusion_texture);
		}
	}

	// Update descriptor sets if we have any writes to perform
	if (!descriptor_writes.empty())
	{
		try
		{
			core_->get_logical_device().updateDescriptorSets(descriptor_writes, {});
			DLOGI("Successfully updated {} descriptors", descriptor_writes.size());
		}
		catch (const std::exception &e)
		{
			LOGE("Failed to update descriptor sets: {}", e.what());
		}
	}
}
const vk::UniqueDescriptorSet *MaterialSystem::get_bindless_descriptor_set() const
{
	return &bindless_descriptor_set_;
}
lz::Buffer *MaterialSystem::get_material_parameters_buffer() const
{
	return material_parameters_buffer_.get();
}
lz::Sampler *MaterialSystem::get_default_sampler() const
{
	return default_sampler_.get();
}
uint32_t MaterialSystem::get_texture_index(const std::shared_ptr<Texture> &texture) const
{
	if (texture == nullptr)
	{
		return UINT32_MAX;
	}
	auto it = texture_name_to_index_.find(texture->name);
	if (it != texture_name_to_index_.end())
	{
		return it->second;
	}
	return UINT32_MAX;
}
}        // namespace lz
