#pragma once
#include <map>
#include <set>
#include "spirv_cross.hpp"
#include <algorithm>

#include "Buffer.h"
#include "ShaderModule.h"

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
		ShaderResourceId()
			: id(size_t(-1))
		{
		}

		// Equality operator
		bool operator==(const ShaderResourceId<Base>& other) const
		{
			return this->id == other.id;
		}

		// IsValid: Checks if this is a valid resource ID
		// Returns: True if the ID is valid, false otherwise
		bool IsValid()
		{
			return id != size_t(-1);
		}

	private:
		// Private constructor used by friend classes
		explicit ShaderResourceId(size_t id) : id(id)
		{
		}

		size_t id;  // Internal ID value
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
		ImageSamplerBinding(lz::ImageView* imageView, lz::Sampler* sampler, uint32_t shaderBindingId);

		// Comparison operator for container ordering
		bool operator<(const ImageSamplerBinding& other) const;

		lz::ImageView* imageView;     // Image view to bind
		lz::Sampler* sampler;         // Sampler to bind
		uint32_t shaderBindingId;     // Binding point in the shader
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
		UniformBufferBinding(lz::Buffer* buffer, uint32_t shaderBindingId, vk::DeviceSize offset, vk::DeviceSize size);

		// Comparison operator for container ordering
		bool operator <(const UniformBufferBinding& other) const;

		lz::Buffer* buffer;        // Buffer to bind
		uint32_t shaderBindingId;  // Binding point in the shader
		vk::DeviceSize offset;     // Offset within the buffer
		vk::DeviceSize size;       // Size of the data to bind
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
		StorageBufferBinding(lz::Buffer* _buffer, uint32_t _shaderBindingId, vk::DeviceSize _offset,
		                     vk::DeviceSize _size);

		// Comparison operator for container ordering
		bool operator <(const StorageBufferBinding& other) const;

		lz::Buffer* buffer;        // Buffer to bind
		uint32_t shaderBindingId;  // Binding point in the shader
		vk::DeviceSize offset;     // Offset within the buffer
		vk::DeviceSize size;       // Size of the data to bind
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
		StorageImageBinding(lz::ImageView* _imageView, uint32_t _shaderBindingId);

		// Comparison operator for container ordering
		bool operator <(const StorageImageBinding& other) const;

		lz::ImageView* imageView;    // Image view to bind
		uint32_t shaderBindingId;    // Binding point in the shader
	};

	// DescriptorSetLayoutKey: Class for managing descriptor set layouts
	// - Provides methods for querying and creating bindings for shader resources
	// - Maintains information about uniform buffers, storage buffers, combined image samplers, etc.
	class DescriptorSetLayoutKey
	{
	public:
		// Type definitions for various shader resource IDs
		struct UniformBase;
		using UniformId = ShaderResourceId<UniformBase>;
		struct UniformBufferBase;
		using UniformBufferId = ShaderResourceId<UniformBufferBase>;
		struct ImageSamplerBase;
		using ImageSamplerId = ShaderResourceId<ImageSamplerBase>;
		struct StorageBufferBase;
		using StorageBufferId = ShaderResourceId<StorageBufferBase>;
		struct StorageImageBase;
		using StorageImageId = ShaderResourceId<StorageImageBase>;

		// UniformData: Structure for individual uniform variable information
		struct UniformData
		{
			// Comparison operator for container ordering
			bool operator<(const UniformData& other) const;

			std::string name;             // Name of the uniform variable
			uint32_t offsetInBinding;     // Offset within the uniform buffer
			uint32_t size;                // Size of the uniform variable
			UniformBufferId uniformBufferId;  // ID of the parent uniform buffer
		};

		// UniformBufferData: Structure for uniform buffer information
		struct UniformBufferData
		{
			// Comparison operator for container ordering
			bool operator<(const UniformBufferData& other) const;

			std::string name;                // Name of the uniform buffer
			uint32_t shaderBindingIndex;     // Binding point in the shader
			vk::ShaderStageFlags stageFlags; // Shader stages that use this uniform buffer

			uint32_t size;                   // Size of the uniform buffer
			uint32_t offsetInSet;            // Offset within the descriptor set
			std::vector<UniformId> uniformIds;  // IDs of uniform variables in this buffer
		};

		// ImageSamplerData: Structure for combined image sampler information
		struct ImageSamplerData
		{
			// Comparison operator for container ordering
			bool operator<(const ImageSamplerData& other) const;

			std::string name;                // Name of the combined image sampler
			uint32_t shaderBindingIndex;     // Binding point in the shader
			vk::ShaderStageFlags stageFlags; // Shader stages that use this image sampler
		};

		// StorageBufferData: Structure for storage buffer information
		struct StorageBufferData
		{
			// Comparison operator for container ordering
			bool operator<(const StorageBufferData& other) const;

			std::string name;
			uint32_t shaderBindingIndex;
			vk::ShaderStageFlags stageFlags;

			uint32_t podPartSize;
			uint32_t arrayMemberSize;
			uint32_t offsetInSet;
		};

		// StorageImageData: Structure for storage image information
		struct StorageImageData
		{
			// Comparison operator for container ordering
			bool operator<(const StorageImageData& other) const;

			std::string name;
			uint32_t shaderBindingIndex;
			vk::ShaderStageFlags stageFlags;
		};

		size_t GetUniformBuffersCount() const;

		void GetUniformBufferIds(UniformBufferId* dstUniformBufferIds, size_t count = -1, size_t offset = 0) const;

		UniformBufferId GetUniformBufferId(std::string bufferName) const;

		UniformBufferId GetUniformBufferId(uint32_t bufferBindingId) const;

		UniformBufferData GetUniformBufferInfo(UniformBufferId uniformBufferId) const;

		UniformBufferBinding MakeUniformBufferBinding(std::string bufferName, lz::Buffer* buffer,
		                                              vk::DeviceSize offset = 0,
		                                              vk::DeviceSize size = VK_WHOLE_SIZE) const;

		size_t GetStorageBuffersCount() const;

		void GetStorageBufferIds(StorageBufferId* dstStorageBufferIds, size_t count = -1, size_t offset = 0) const;

		StorageBufferId GetStorageBufferId(std::string storageBufferName) const;

		StorageBufferId GetStorageBufferId(uint32_t bufferBindingId) const;

		StorageBufferData GetStorageBufferInfo(StorageBufferId storageBufferId) const;

		StorageBufferBinding MakeStorageBufferBinding(std::string bufferName, lz::Buffer* _buffer,
		                                              vk::DeviceSize _offset = 0,
		                                              vk::DeviceSize _size = VK_WHOLE_SIZE) const;

		template <typename MemberType>
		StorageBufferBinding MakeCheckedStorageBufferBinding(std::string bufferName,
			lz::Buffer* _buffer, vk::DeviceSize _offset, vk::DeviceSize _size) const
		{
			auto storageBufferId = GetStorageBufferId(bufferName);
			assert(storageBufferId.IsValid());
			auto storageBufferInfo = GetStorageBufferInfo(storageBufferId);
			assert(storageBufferInfo.arrayMemberSize == sizeof(MemberType));
			return StorageBufferBinding(_buffer, storageBufferInfo.shaderBindingIndex, _offset, _size);
		}

		size_t GetUniformsCount() const;

		void GetUniformIds(UniformId* dstUniformIds, size_t count = -1, size_t offset = 0) const;

		UniformData GetUniformInfo(UniformId uniformId) const;

		UniformId GetUniformId(std::string name) const;

		size_t GetImageSamplersCount() const;

		void GetImageSamplerIds(ImageSamplerId* dstImageSamplerIds, size_t count = -1, size_t offset = 0) const;

		ImageSamplerData GetImageSamplerInfo(ImageSamplerId imageSamplerId) const;

		ImageSamplerId GetImageSamplerId(std::string imageSamplerName) const;

		ImageSamplerId GetImageSamplerId(uint32_t shaderBindingIndex) const;

		ImageSamplerBinding MakeImageSamplerBinding(std::string imageSamplerName, lz::ImageView* _imageView,
		                                            lz::Sampler* _sampler) const;

		size_t GetStorageImagesCount() const;

		void GetStorageImageIds(StorageImageId* dstStorageImageIds, size_t count = -1, size_t offset = 0) const;

		StorageImageId GetStorageImageId(std::string storageImageName) const;

		StorageImageId GetStorageImageId(uint32_t bufferBindingId) const;

		StorageImageData GetStorageImageInfo(StorageImageId storageImageId) const;

		StorageImageBinding MakeStorageImageBinding(std::string imageName, lz::ImageView* _imageView) const;

		uint32_t GetTotalConstantBufferSize() const;

		bool IsEmpty() const;

		bool operator<(const DescriptorSetLayoutKey& other) const;

		static DescriptorSetLayoutKey Merge(DescriptorSetLayoutKey* setLayouts, size_t setsCount);

	private:
		void RebuildIndex();


		friend class Shader;
		uint32_t setShaderId;
		uint32_t size;

		std::vector<UniformData> uniformDatum;
		std::vector<UniformBufferData> uniformBufferDatum;
		std::vector<ImageSamplerData> imageSamplerDatum;
		std::vector<StorageBufferData> storageBufferDatum;
		std::vector<StorageImageData> storageImageDatum;

		std::map<std::string, UniformId> uniformNameToIds;
		std::map<std::string, UniformBufferId> uniformBufferNameToIds;
		std::map<uint32_t, UniformBufferId> uniformBufferBindingToIds;
		std::map<std::string, ImageSamplerId> imageSamplerNameToIds;
		std::map<uint32_t, ImageSamplerId> imageSamplerBindingToIds;
		std::map<std::string, StorageBufferId> storageBufferNameToIds;
		std::map<uint32_t, StorageBufferId> storageBufferBindingToIds;
		std::map<std::string, StorageImageId> storageImageNameToIds;
		std::map<uint32_t, StorageImageId> storageImageBindingToIds;
	};

	class Shader
	{
	public:
		Shader(vk::Device logicalDevice, std::string shaderFile);

		Shader(vk::Device logicalDevice, const std::vector<uint32_t>& bytecode);

		static const std::vector<uint32_t> GetBytecode(std::string filename);

		lz::ShaderModule* GetModule();


		size_t GetSetsCount();

		const DescriptorSetLayoutKey* GetSetInfo(size_t setIndex);

		glm::uvec3 GetLocalSize();

	private:
		void Init(vk::Device logicalDevice, const std::vector<uint32_t>& bytecode);

		std::vector<DescriptorSetLayoutKey> descriptorSetLayoutKeys;

		std::unique_ptr<lz::ShaderModule> shaderModule;
		glm::uvec3 localSize;
	};

	class ShaderProgram
	{
	public:
		ShaderProgram(Shader* _vertexShader, Shader* _fragmentShader);

		size_t GetSetsCount();

		const DescriptorSetLayoutKey* GetSetInfo(size_t setIndex);

		std::vector<DescriptorSetLayoutKey> combinedDescriptorSetLayoutKeys;
		Shader* vertexShader;
		Shader* fragmentShader;
	};
}
