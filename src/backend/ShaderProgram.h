#pragma once
#include "spirv_cross.hpp"
#include <algorithm>
#include <map>
#include <set>

#include "Buffer.h"
#include "ShaderModule.h"
#include "glm/glm.hpp"

namespace lz
{
class DescriptorSetLayoutKey;
class Shader;
class Sampler;
class ImageView;
class Buffer;

// ShaderResourceId: Template class for type-safe identifiers of shader resources
// - Provides strong typing to prevent mixing different resource types
// - Used to reference resources like uniforms, samplers, etc.
template <typename Base>
struct ShaderResourceId
{
	// Default constructor: Creates an invalid resource ID
	ShaderResourceId() :
	    id_(size_t(-1))
	{
	}

	// Equality operator
	bool operator==(const ShaderResourceId<Base> &other) const
	{
		return this->id_ == other.id_;
	}

	// IsValid: Checks if this is a valid resource ID
	// Returns: True if the ID is valid, false otherwise
	bool is_valid() const
	{
		return id_ != size_t(-1);
	}

  private:
	// Private constructor used by friend classes
	explicit ShaderResourceId(size_t id) :
	    id_(id)
	{
	}

	size_t id_;        // Internal ID value
	friend class DescriptorSetLayoutKey;
	friend class Shader;
};

// ImageSamplerBinding: Structure for combined image sampler binding data
// - Represents a binding between an image view and a sampler for use in shaders
struct ImageSamplerBinding
{
	// Default constructor: Creates an empty binding
	ImageSamplerBinding();

	// Constructor: Creates a binding with specified image view and sampler
	// Parameters:
	// - imageView: Image view to bind
	// - sampler: Sampler to bind
	// - shaderBindingId: Binding point in the shader
	ImageSamplerBinding(lz::ImageView *image_view, lz::Sampler *sampler, uint32_t shader_binding_id);

	// Comparison operator for container ordering
	bool operator<(const ImageSamplerBinding &other) const;

	lz::ImageView *image_view;               // Image view to bind
	lz::Sampler   *sampler;                  // Sampler to bind
	uint32_t       shader_binding_id;        // Binding point in the shader
};

// UniformBufferBinding: Structure for uniform buffer binding data
// - Represents a binding between a buffer and a uniform buffer binding point in the shader
struct UniformBufferBinding
{
	// Default constructor: Creates an empty binding
	UniformBufferBinding();

	// Constructor: Creates a binding with specified buffer
	// Parameters:
	// - buffer: Buffer to bind
	// - shaderBindingId: Binding point in the shader
	// - offset: Offset within the buffer
	// - size: Size of the data to bind
	UniformBufferBinding(lz::Buffer *buffer, uint32_t shader_binding_id, vk::DeviceSize offset,
	                     vk::DeviceSize size);

	// Comparison operator for container ordering
	bool operator<(const UniformBufferBinding &other) const;

	lz::Buffer    *buffer;                   // Buffer to bind
	uint32_t       shader_binding_id;        // Binding point in the shader
	vk::DeviceSize offset;                   // Offset within the buffer
	vk::DeviceSize size;                     // Size of the data to bind
};

// StorageBufferBinding: Structure for storage buffer binding data
// - Represents a binding between a buffer and a storage buffer binding point in the shader
struct StorageBufferBinding
{
	// Default constructor: Creates an empty binding
	StorageBufferBinding();

	// Constructor: Creates a binding with specified buffer
	// Parameters:
	// - _buffer: Buffer to bind
	// - _shaderBindingId: Binding point in the shader
	// - _offset: Offset within the buffer
	// - _size: Size of the data to bind
	StorageBufferBinding(lz::Buffer *buffer, uint32_t shader_binding_id, vk::DeviceSize offset,
	                     vk::DeviceSize size);

	// Comparison operator for container ordering
	bool operator<(const StorageBufferBinding &other) const;

	lz::Buffer    *buffer;                   // Buffer to bind
	uint32_t       shader_binding_id;        // Binding point in the shader
	vk::DeviceSize offset;                   // Offset within the buffer
	vk::DeviceSize size;                     // Size of the data to bind
};

// StorageImageBinding: Structure for storage image binding data
// - Represents a binding between an image view and a storage image binding point in the shader
struct StorageImageBinding
{
	// Default constructor: Creates an empty binding
	StorageImageBinding();

	// Constructor: Creates a binding with specified image view
	// Parameters:
	// - _imageView: Image view to bind
	// - _shaderBindingId: Binding point in the shader
	StorageImageBinding(lz::ImageView *image_view, uint32_t shader_binding_id);

	// Comparison operator for container ordering
	bool operator<(const StorageImageBinding &other) const;

	lz::ImageView *image_view;               // Image view to bind
	uint32_t       shader_binding_id;        // Binding point in the shader
};

// DescriptorSetLayoutKey: Class for managing descriptor set layouts
// - Provides methods for querying and creating bindings for shader resources
// - Maintains information about uniform buffers, storage buffers, combined image samplers, etc.
class DescriptorSetLayoutKey
{
  public:
	// Type definitions for various shader resource IDs
	struct UniformBase;
	struct UniformBufferBase;
	struct ImageSamplerBase;
	struct StorageBufferBase;
	struct StorageImageBase;

	using UniformBufferId = ShaderResourceId<UniformBufferBase>;
	using ImageSamplerId  = ShaderResourceId<ImageSamplerBase>;
	using StorageBufferId = ShaderResourceId<StorageBufferBase>;
	using UniformId       = ShaderResourceId<UniformBase>;
	using StorageImageId  = ShaderResourceId<StorageImageBase>;

	// UniformData: Structure for individual uniform variable information
	struct UniformData
	{
		// Comparison operator for container ordering
		bool operator<(const UniformData &other) const;

		std::string     name;                     // Name of the uniform variable
		uint32_t        offset_in_binding;        // Offset within the uniform buffer
		uint32_t        size;                     // Size of the uniform variable
		UniformBufferId uniform_buffer_id;        // ID of the parent uniform buffer
	};

	// UniformBufferData: Structure for uniform buffer information
	struct UniformBufferData
	{
		// Comparison operator for container ordering
		bool operator<(const UniformBufferData &other) const;

		std::string          name;                        // Name of the uniform buffer
		uint32_t             shader_binding_index;        // Binding point in the shader
		vk::ShaderStageFlags stage_flags;                 // Shader stages that use this uniform buffer

		uint32_t               size;                 // Size of the uniform buffer
		uint32_t               offset_in_set;        // Offset within the descriptor set
		std::vector<UniformId> uniform_ids;          // IDs of uniform variables in this buffer
	};

	// ImageSamplerData: Structure for combined image sampler information
	struct ImageSamplerData
	{
		// Comparison operator for container ordering
		bool operator<(const ImageSamplerData &other) const;

		std::string          name;                        // Name of the combined image sampler
		uint32_t             shader_binding_index;        // Binding point in the shader
		vk::ShaderStageFlags stage_flags;                 // Shader stages that use this image sampler
	};

	// StorageBufferData: Structure for storage buffer information
	struct StorageBufferData
	{
		// Comparison operator for container ordering
		bool operator<(const StorageBufferData &other) const;

		std::string          name;
		uint32_t             shader_binding_index;
		vk::ShaderStageFlags stage_flags;

		uint32_t pod_part_size;
		uint32_t array_member_size;
		uint32_t offset_in_set;
	};

	// StorageImageData: Structure for storage image information
	struct StorageImageData
	{
		// Comparison operator for container ordering
		bool operator<(const StorageImageData &other) const;

		std::string          name;
		uint32_t             shader_binding_index;
		vk::ShaderStageFlags stage_flags;
	};

	size_t get_uniform_buffers_count() const;

	void get_uniform_buffer_ids(UniformBufferId *dst_uniform_buffer_ids, size_t count = -1, size_t offset = 0) const;

	UniformBufferId get_uniform_buffer_id(std::string buffer_name) const;

	UniformBufferId get_uniform_buffer_id(uint32_t buffer_binding_id) const;

	UniformBufferData get_uniform_buffer_info(UniformBufferId uniform_buffer_id) const;

	UniformBufferBinding make_uniform_buffer_binding(std::string buffer_name, lz::Buffer *buffer,
	                                                 vk::DeviceSize offset = 0,
	                                                 vk::DeviceSize size   = VK_WHOLE_SIZE) const;

	size_t get_storage_buffers_count() const;

	void get_storage_buffer_ids(StorageBufferId *dst_storage_buffer_ids, size_t count = -1, size_t offset = 0) const;

	StorageBufferId get_storage_buffer_id(std::string storage_buffer_name) const;

	StorageBufferId get_storage_buffer_id(uint32_t buffer_binding_id) const;

	StorageBufferData get_storage_buffer_info(StorageBufferId storage_buffer_id) const;

	StorageBufferBinding make_storage_buffer_binding(std::string buffer_name, lz::Buffer *buffer,
	                                                 vk::DeviceSize offset = 0,
	                                                 vk::DeviceSize size   = VK_WHOLE_SIZE) const;

	template <typename MemberType>
	StorageBufferBinding make_checked_storage_buffer_binding(std::string buffer_name,
	                                                         lz::Buffer *buffer, vk::DeviceSize offset,
	                                                         vk::DeviceSize size) const
	{
		auto storage_buffer_id = get_storage_buffer_id(buffer_name);
		assert(storage_buffer_id.is_valid());
		auto storage_buffer_info = get_storage_buffer_info(storage_buffer_id);
		assert(storage_buffer_info.array_member_size == sizeof(MemberType));
		return StorageBufferBinding(buffer, storage_buffer_info.shader_binding_index, offset, size);
	}

	size_t get_uniforms_count() const;

	void get_uniform_ids(UniformId *dst_uniform_ids, size_t count = -1, size_t offset = 0) const;

	UniformData get_uniform_info(UniformId uniform_id) const;

	UniformId get_uniform_id(std::string name) const;

	size_t get_image_samplers_count() const;

	void get_image_sampler_ids(ImageSamplerId *dst_image_sampler_ids, size_t count = -1, size_t offset = 0) const;

	ImageSamplerData get_image_sampler_info(ImageSamplerId image_sampler_id) const;

	ImageSamplerId get_image_sampler_id(std::string image_sampler_name) const;
	ImageSamplerId get_image_sampler_id(uint32_t shader_binding_index) const;

	ImageSamplerBinding make_image_sampler_binding(std::string image_sampler_name, lz::ImageView *image_view,
	                                               lz::Sampler *sampler) const;

	size_t get_storage_images_count() const;

	void get_storage_image_ids(StorageImageId *dst_storage_image_ids, size_t count = -1, size_t offset = 0) const;

	StorageImageId get_storage_image_id(std::string storage_image_name) const;

	StorageImageId get_storage_image_id(uint32_t buffer_binding_id) const;

	StorageImageData get_storage_image_info(StorageImageId storage_image_id) const;

	StorageImageBinding make_storage_image_binding(std::string image_name, lz::ImageView *image_view) const;

	uint32_t get_total_constant_buffer_size() const;

	bool is_empty() const;

	bool operator<(const DescriptorSetLayoutKey &other) const;

	static DescriptorSetLayoutKey merge(DescriptorSetLayoutKey *set_layouts, size_t sets_count);

	uint32_t get_set_id() const;

  private:
	void rebuild_index();

	friend class Shader;
	uint32_t set_shader_id_;
	uint32_t size_;

	std::vector<UniformData>       uniform_datum_;
	std::vector<UniformBufferData> uniform_buffer_datum_;
	std::vector<ImageSamplerData>  image_sampler_datum_;
	std::vector<StorageBufferData> storage_buffer_datum_;
	std::vector<StorageImageData>  storage_image_datum_;

	std::map<std::string, UniformId>       uniform_name_to_ids_;
	std::map<std::string, UniformBufferId> uniform_buffer_name_to_ids_;
	std::map<uint32_t, UniformBufferId>    uniform_buffer_binding_to_ids_;
	std::map<std::string, ImageSamplerId>  image_sampler_name_to_ids_;
	std::map<uint32_t, ImageSamplerId>     image_sampler_binding_to_ids_;
	std::map<std::string, StorageBufferId> storage_buffer_name_to_ids_;
	std::map<uint32_t, StorageBufferId>    storage_buffer_binding_to_ids_;
	std::map<std::string, StorageImageId>  storage_image_name_to_ids_;
	std::map<uint32_t, StorageImageId>     storage_image_binding_to_ids_;
};

class Shader
{
  public:
	Shader(vk::Device logical_device, std::string shader_file);

	Shader(vk::Device logical_device, const std::vector<uint32_t> &bytecode);

	static const std::vector<uint32_t> get_bytecode(std::string filename);

	lz::ShaderModule *get_module();

	vk::ShaderStageFlagBits get_stage_bits() const;

	size_t get_sets_count();

	const DescriptorSetLayoutKey *get_set_info(size_t set_index);

	glm::uvec3 get_local_size();

  private:
	void init(vk::Device logical_device, const std::vector<uint32_t> &bytecode);

	std::vector<DescriptorSetLayoutKey> descriptor_set_layout_keys_;
	vk::ShaderStageFlagBits             stage_flag_bits_;

	std::unique_ptr<lz::ShaderModule> shader_module_;
	glm::uvec3                        local_size_;
};

class ShaderProgram
{
  public:
	ShaderProgram(std::initializer_list<Shader *> shaders);

	size_t get_sets_count();

	const DescriptorSetLayoutKey *get_set_info(size_t set_index);

	std::vector<DescriptorSetLayoutKey> combined_descriptor_set_layout_keys;

	std::vector<Shader *> shaders;
};
}        // namespace lz
