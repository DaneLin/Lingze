#pragma once

#include "backend/Config.h"
#include "backend/Image.h"
#include "backend/ImageView.h"
#include "backend/Sampler.h"
#include "glm/glm.hpp"
#include <backend/StagedResources.h>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

namespace lz
{

class Core;
/**
 * @brief Texture is a class that contains image data
 */
class Texture
{
  public:
	Texture()  = default;
	~Texture() = default;

	int                        width{-1};
	int                        height{-1};
	int                        channels{-1};
	std::vector<unsigned char> data;
	std::string                name;
	std::string                uri;
};

/**
 * @brief Material is a material class that contains different types of texture maps
 */
class Material
{
  public:
	Material()  = default;
	~Material() = default;

	std::string              name;
	std::shared_ptr<Texture> diffuse_texture;
	std::shared_ptr<Texture> normal_texture;
	std::shared_ptr<Texture> metallic_roughness_texture;
	std::shared_ptr<Texture> emissive_texture;
	std::shared_ptr<Texture> occlusion_texture;

	// PBR parameters
	glm::vec4 base_color_factor{1.0f};
	float     metallic_factor{1.0f};
	float     roughness_factor{1.0f};
	glm::vec3 emissive_factor{0.0f};
};

struct alignas(16) MaterialParameters
{
	glm::vec4 base_color_factor;
	glm::vec3 emissive_factor;
	float     metallic_factor;
	float     roughness_factor;
	uint32_t  diffuse_texture_index;
	uint32_t  normal_texture_index;
	uint32_t  metallic_roughness_texture_index;
	uint32_t  emissive_texture_index;
	uint32_t  occlusion_texture_index;
};

struct UpdateRequest
{
	enum class UpdateType
	{
		eTextureUpload,
		eMaterialParametersUpdate,
		eMaterialDelete
	};

	UpdateType               type;
	std::string              material_name;
	std::shared_ptr<Texture> texture;
};

class MaterialSystem
{
  public:
	MaterialSystem(Core *core);
	~MaterialSystem();

	uint32_t                       register_material(const std::shared_ptr<Material> &material);
	uint32_t                       get_material_index(const std::string &material_name) const;
	uint32_t                       upload_texture(const std::shared_ptr<Texture> &texture);
	void                           request_update(const UpdateRequest &request);
	void                           process_pending_updates();
	const vk::UniqueDescriptorSet *get_bindless_descriptor_set() const;
	lz::Buffer                    *get_material_parameters_buffer() const;
	lz::Sampler                   *get_default_sampler() const;

  private:
	void     initialize();
	uint32_t allocate_texture_slot();
	uint32_t allocate_material_slot();
	void     update_material_parameters(const std::shared_ptr<Material> &material);

	uint32_t get_texture_index(const std::shared_ptr<Texture> &texture) const;

  private:
	lz::Core *core_;

	vk::UniqueDescriptorSetLayout bindless_descriptor_set_layout_;
	vk::UniqueDescriptorPool      bindless_descriptor_pool_;
	vk::UniqueDescriptorSet       bindless_descriptor_set_;

	std::vector<std::unique_ptr<Image>>       texture_images_;
	std::vector<std::unique_ptr<ImageView>>   texture_views_;
	std::unordered_map<std::string, uint32_t> texture_name_to_index_;
	uint32_t                                  texture_count_ = 0;

	std::unique_ptr<lz::Sampler> default_sampler_;

	std::vector<std::shared_ptr<Material>>    materials_;
	std::unordered_map<std::string, uint32_t> material_name_to_index_;
	uint32_t                                  material_count_{0};

	std::unique_ptr<lz::Buffer> staging_buffer_;
	std::unique_ptr<lz::Buffer> material_parameters_buffer_;

	std::queue<UpdateRequest> pending_updates_;

	std::vector<uint32_t> free_texture_slots_;
	std::vector<uint32_t> free_material_slots_;
};
}        // namespace lz