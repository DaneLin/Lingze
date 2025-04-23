#pragma once
#include <map>
#include <set>
#include "spirv_cross.hpp"
#include <algorithm>

#include "Buffer.h"

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
		ImageSamplerBinding() : imageView(nullptr), sampler(nullptr)
		{
		}

		// Constructor: Creates a binding with specified image view and sampler
		// Parameters:
		// - imageView: Image view to bind
		// - sampler: Sampler to bind
		// - shaderBindingId: Binding point in the shader
		ImageSamplerBinding(lz::ImageView* imageView, lz::Sampler* sampler, uint32_t shaderBindingId)
			: imageView(imageView), sampler(sampler), shaderBindingId(shaderBindingId)
		{
			assert(imageView);
			assert(sampler);
		}

		// Comparison operator for container ordering
		bool operator<(const ImageSamplerBinding& other) const
		{
			return std::tie(imageView, sampler, shaderBindingId) < std::tie(
				other.imageView, other.sampler, other.shaderBindingId);
		}

		lz::ImageView* imageView;     // Image view to bind
		lz::Sampler* sampler;         // Sampler to bind
		uint32_t shaderBindingId;     // Binding point in the shader
	};

	// UniformBufferBinding: Structure for uniform buffer binding data
	// - Represents a binding between a buffer and a uniform buffer binding point in the shader
	struct UniformBufferBinding
	{
		// Default constructor: Creates an empty binding
		UniformBufferBinding() : buffer(nullptr), offset(-1), size(-1)
		{
		}

		// Constructor: Creates a binding with specified buffer
		// Parameters:
		// - buffer: Buffer to bind
		// - shaderBindingId: Binding point in the shader
		// - offset: Offset within the buffer
		// - size: Size of the data to bind
		UniformBufferBinding(lz::Buffer* buffer, uint32_t shaderBindingId, vk::DeviceSize offset, vk::DeviceSize size)
			: buffer(buffer), shaderBindingId(shaderBindingId), offset(offset), size(size)
		{
			assert(buffer);
		}

		// Comparison operator for container ordering
		bool operator <(const UniformBufferBinding& other) const
		{
			return std::tie(buffer, shaderBindingId, offset, size) < std::tie(
				other.buffer, other.shaderBindingId, other.offset, other.size);
		}

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
		StorageBufferBinding() : buffer(nullptr), offset(-1), size(-1)
		{
		}

		// Constructor: Creates a binding with specified buffer
		// Parameters:
		// - _buffer: Buffer to bind
		// - _shaderBindingId: Binding point in the shader
		// - _offset: Offset within the buffer
		// - _size: Size of the data to bind
		StorageBufferBinding(lz::Buffer* _buffer, uint32_t _shaderBindingId, vk::DeviceSize _offset,
		                     vk::DeviceSize _size) : buffer(_buffer), shaderBindingId(_shaderBindingId),
		                                             offset(_offset), size(_size)
		{
			assert(_buffer);
		}

		// Comparison operator for container ordering
		bool operator <(const StorageBufferBinding& other) const
		{
			return std::tie(buffer, shaderBindingId, offset, size) < std::tie(
				other.buffer, other.shaderBindingId, other.offset, other.size);
		}

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
		StorageImageBinding() : imageView(nullptr)
		{
		}

		// Constructor: Creates a binding with specified image view
		// Parameters:
		// - _imageView: Image view to bind
		// - _shaderBindingId: Binding point in the shader
		StorageImageBinding(lz::ImageView* _imageView, uint32_t _shaderBindingId) : imageView(_imageView),
			shaderBindingId(_shaderBindingId)
		{
			assert(_imageView);
		}

		// Comparison operator for container ordering
		bool operator <(const StorageImageBinding& other) const
		{
			return std::tie(imageView, shaderBindingId) < std::tie(other.imageView, other.shaderBindingId);
		}

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
			bool operator<(const UniformData& other) const
			{
				return std::tie(name, offsetInBinding, size) < std::tie(other.name, other.offsetInBinding, other.size);
			}

			std::string name;             // Name of the uniform variable
			uint32_t offsetInBinding;     // Offset within the uniform buffer
			uint32_t size;                // Size of the uniform variable
			UniformBufferId uniformBufferId;  // ID of the parent uniform buffer
		};

		// UniformBufferData: Structure for uniform buffer information
		struct UniformBufferData
		{
			// Comparison operator for container ordering
			bool operator<(const UniformBufferData& other) const
			{
				return std::tie(name, shaderBindingIndex, size) < std::tie(
					other.name, other.shaderBindingIndex, other.size);
			}

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
			bool operator<(const ImageSamplerData& other) const
			{
				return std::tie(name, shaderBindingIndex) < std::tie(other.name, other.shaderBindingIndex);
			}

			std::string name;                // Name of the combined image sampler
			uint32_t shaderBindingIndex;     // Binding point in the shader
			vk::ShaderStageFlags stageFlags; // Shader stages that use this image sampler
		};

		// StorageBufferData: Structure for storage buffer information
		struct StorageBufferData
		{
			// Comparison operator for container ordering
			bool operator<(const StorageBufferData& other) const
			{
				return std::tie(name, shaderBindingIndex, podPartSize, arrayMemberSize) < std::tie(
					other.name, other.shaderBindingIndex, other.podPartSize, other.arrayMemberSize);
			}

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
			bool operator<(const StorageImageData& other) const
			{
				return std::tie(name, shaderBindingIndex) < std::tie(other.name, other.shaderBindingIndex);
			}

			std::string name;
			uint32_t shaderBindingIndex;
			vk::ShaderStageFlags stageFlags;
		};

		size_t GetUniformBuffersCount() const
		{
			return uniformBufferDatum.size();
		}

		void GetUniformBufferIds(UniformBufferId* dstUniformBufferIds, size_t count = -1, size_t offset = 0) const
		{
			if (count == -1)
			{
				count = uniformBufferDatum.size();
			}
			assert(count + offset <= uniformBufferDatum.size());
			for (size_t index = offset; index < offset + count; ++index)
			{
				dstUniformBufferIds[index] = UniformBufferId(index);
			}
		}

		UniformBufferId GetUniformBufferId(std::string bufferName) const
		{
			auto it = uniformBufferNameToIds.find(bufferName);
			if (it == uniformBufferNameToIds.end())
			{
				return UniformBufferId();
			}
			return it->second;
		}

		UniformBufferId GetUniformBufferId(uint32_t bufferBindingId) const
		{
			auto it = uniformBufferBindingToIds.find(bufferBindingId);
			if (it == uniformBufferBindingToIds.end())
			{
				return UniformBufferId();
			}
			return it->second;
		}

		UniformBufferData GetUniformBufferInfo(UniformBufferId uniformBufferId) const
		{
			return uniformBufferDatum[uniformBufferId.id];
		}

		UniformBufferBinding MakeUniformBufferBinding(std::string bufferName, lz::Buffer* buffer,
		                                              vk::DeviceSize offset = 0,
		                                              vk::DeviceSize size = VK_WHOLE_SIZE) const
		{
			auto uniformBufferId = GetUniformBufferId(bufferName);
			assert(uniformBufferId.IsValid());
			auto uniformBufferInfo = GetUniformBufferInfo(uniformBufferId);
			return UniformBufferBinding(buffer, uniformBufferInfo.shaderBindingIndex, offset, size);
		}

		size_t GetStorageBuffersCount() const
		{
			return storageBufferDatum.size();
		}

		void GetStorageBufferIds(StorageBufferId* dstStorageBufferIds, size_t count = -1, size_t offset = 0) const
		{
			if (count == -1)
				count = storageBufferDatum.size();
			assert(count + offset <= storageBufferDatum.size());
			for (size_t index = offset; index < offset + count; index++)
				dstStorageBufferIds[index] = StorageBufferId(index);
		}

		StorageBufferId GetStorageBufferId(std::string storageBufferName) const
		{
			auto it = storageBufferNameToIds.find(storageBufferName);
			if (it == storageBufferNameToIds.end())
				return StorageBufferId();
			return it->second;
		}

		StorageBufferId GetStorageBufferId(uint32_t bufferBindingId) const
		{
			auto it = storageBufferBindingToIds.find(bufferBindingId);
			if (it == storageBufferBindingToIds.end())
				return StorageBufferId();
			return it->second;
		}

		StorageBufferData GetStorageBufferInfo(StorageBufferId storageBufferId) const
		{
			return storageBufferDatum[storageBufferId.id];
		}

		StorageBufferBinding MakeStorageBufferBinding(std::string bufferName, lz::Buffer* _buffer,
		                                              vk::DeviceSize _offset = 0,
		                                              vk::DeviceSize _size = VK_WHOLE_SIZE) const
		{
			auto storageBufferId = GetStorageBufferId(bufferName);
			assert(storageBufferId.IsValid());
			auto storageBufferInfo = GetStorageBufferInfo(storageBufferId);
			return StorageBufferBinding(_buffer, storageBufferInfo.shaderBindingIndex, _offset, _size);
		}

		template <typename MemberType>
		StorageBufferBinding MakeCheckedStorageBufferBinding(std::string bufferName, lz::Buffer* _buffer,
		                                                     vk::DeviceSize _offset = 0,
		                                                     vk::DeviceSize _size = VK_WHOLE_SIZE) const
		{
			auto storageBufferId = GetStorageBufferId(bufferName);
			assert(storageBufferId.IsValid());
			auto storageBufferInfo = GetStorageBufferInfo(storageBufferId);
			assert(storageBufferInfo.arrayMemberSize == sizeof(MemberType));
			return StorageBufferBinding(_buffer, storageBufferInfo.shaderBindingIndex, _offset, _size);
		}

		size_t GetUniformsCount() const
		{
			return uniformDatum.size();
		}

		void GetUniformIds(UniformId* dstUniformIds, size_t count = -1, size_t offset = 0) const
		{
			if (count == -1)
			{
				count = uniformDatum.size();
			}
			assert(count + offset <= uniformDatum.size());
			for (size_t index = offset; index < offset + count; ++index)
			{
				dstUniformIds[index] = UniformId(index);
			}
		}

		UniformData GetUniformInfo(UniformId uniformId) const
		{
			return uniformDatum[uniformId.id];
		}

		UniformId GetUniformId(std::string name) const
		{
			auto it = uniformNameToIds.find(name);
			if (it == uniformNameToIds.end())
				return UniformId();
			return it->second;
		}

		size_t GetImageSamplersCount() const
		{
			return imageSamplerDatum.size();
		}

		void GetImageSamplerIds(ImageSamplerId* dstImageSamplerIds, size_t count = -1, size_t offset = 0) const
		{
			if (count == -1)
				count = imageSamplerDatum.size();
			assert(count + offset <= imageSamplerDatum.size());
			for (size_t index = offset; index < offset + count; index++)
				dstImageSamplerIds[index] = ImageSamplerId(index);
		}

		ImageSamplerData GetImageSamplerInfo(ImageSamplerId imageSamplerId) const
		{
			return imageSamplerDatum[imageSamplerId.id];
		}

		ImageSamplerId GetImageSamplerId(std::string imageSamplerName) const
		{
			auto it = imageSamplerNameToIds.find(imageSamplerName);
			if (it == imageSamplerNameToIds.end())
				return ImageSamplerId();
			return it->second;
		}

		ImageSamplerId GetImageSamplerId(uint32_t shaderBindingIndex) const
		{
			auto it = imageSamplerBindingToIds.find(shaderBindingIndex);
			if (it == imageSamplerBindingToIds.end())
				return ImageSamplerId();
			return it->second;
		}

		ImageSamplerBinding MakeImageSamplerBinding(std::string imageSamplerName, lz::ImageView* _imageView,
		                                            lz::Sampler* _sampler) const
		{
			auto imageSamplerId = GetImageSamplerId(imageSamplerName);
			assert(imageSamplerId.IsValid());
			auto imageSamplerInfo = GetImageSamplerInfo(imageSamplerId);
			return ImageSamplerBinding(_imageView, _sampler, imageSamplerInfo.shaderBindingIndex);
		}

		size_t GetStorageImagesCount() const
		{
			return storageImageDatum.size();
		}

		void GetStorageImageIds(StorageImageId* dstStorageImageIds, size_t count = -1, size_t offset = 0) const
		{
			if (count == -1)
				count = storageImageDatum.size();
			assert(count + offset <= storageImageDatum.size());
			for (size_t index = offset; index < offset + count; index++)
				dstStorageImageIds[index] = StorageImageId(index);
		}

		StorageImageId GetStorageImageId(std::string storageImageName) const
		{
			auto it = storageImageNameToIds.find(storageImageName);
			if (it == storageImageNameToIds.end())
				return StorageImageId();
			return it->second;
		}

		StorageImageId GetStorageImageId(uint32_t bufferBindingId) const
		{
			auto it = storageImageBindingToIds.find(bufferBindingId);
			if (it == storageImageBindingToIds.end())
				return StorageImageId();
			return it->second;
		}

		StorageImageData GetStorageImageInfo(StorageImageId storageImageId) const
		{
			return storageImageDatum[storageImageId.id];
		}

		StorageImageBinding MakeStorageImageBinding(std::string imageName, lz::ImageView* _imageView) const
		{
			auto imageId = GetStorageImageId(imageName);
			assert(imageId.IsValid());
			auto storageImageInfo = GetStorageImageInfo(imageId);
			return StorageImageBinding(_imageView, storageImageInfo.shaderBindingIndex);
		}

		uint32_t GetTotalConstantBufferSize() const
		{
			return size;
		}

		bool IsEmpty() const
		{
			return GetImageSamplersCount() == 0 && GetUniformBuffersCount() == 0 && GetStorageImagesCount() == 0 &&
				GetStorageBuffersCount() == 0;
		}

		bool operator<(const DescriptorSetLayoutKey& other) const
		{
			return std::tie(uniformDatum, uniformBufferDatum, imageSamplerDatum, storageBufferDatum, storageImageDatum)
				< std::tie(other.uniformDatum, other.uniformBufferDatum, other.imageSamplerDatum,
				           other.storageBufferDatum, other.storageImageDatum);
		}

		static DescriptorSetLayoutKey Merge(DescriptorSetLayoutKey* setLayouts, size_t setsCount)
		{
			DescriptorSetLayoutKey res;
			if (setsCount <= 0) return res;

			res.setShaderId = setLayouts[0].setShaderId;
			for (size_t layoutIndex = 0; layoutIndex < setsCount; layoutIndex++)
				assert(setLayouts[layoutIndex].setShaderId == res.setShaderId);

			std::set<uint32_t> uniformBufferBindings;
			std::set<uint32_t> imageSamplerBindings;
			std::set<uint32_t> storageBufferBindings;
			std::set<uint32_t> storageImageBindings;

			for (size_t setIndex = 0; setIndex < setsCount; setIndex++)
			{
				auto& setLayout = setLayouts[setIndex];
				for (auto& uniformBufferData : setLayout.uniformBufferDatum)
				{
					uniformBufferBindings.insert(uniformBufferData.shaderBindingIndex);
				}
				for (auto& imageSamplerData : setLayout.imageSamplerDatum)
				{
					imageSamplerBindings.insert(imageSamplerData.shaderBindingIndex);
				}
				for (auto& storageBufferData : setLayout.storageBufferDatum)
				{
					storageBufferBindings.insert(storageBufferData.shaderBindingIndex);
				}
				for (auto& storageImageData : setLayout.storageImageDatum)
				{
					storageImageBindings.insert(storageImageData.shaderBindingIndex);
				}
			}

			for (auto& uniformBufferBinding : uniformBufferBindings)
			{
				UniformBufferId dstUniformBufferId;
				for (size_t setIndex = 0; setIndex < setsCount; setIndex++)
				{
					auto& srcLayout = setLayouts[setIndex];
					auto srcUniformBufferId = srcLayout.GetUniformBufferId(uniformBufferBinding);
					if (!srcUniformBufferId.IsValid()) continue;
					const auto& srcUniformBuffer = srcLayout.uniformBufferDatum[srcUniformBufferId.id];
					assert(srcUniformBuffer.shaderBindingIndex == uniformBufferBinding);

					if (!dstUniformBufferId.IsValid())
					{
						dstUniformBufferId = UniformBufferId(res.uniformBufferDatum.size());
						res.uniformBufferDatum.push_back(UniformBufferData());
						auto& dstUniformBuffer = res.uniformBufferDatum.back();

						dstUniformBuffer.shaderBindingIndex = srcUniformBuffer.shaderBindingIndex;
						dstUniformBuffer.name = srcUniformBuffer.name;
						dstUniformBuffer.size = srcUniformBuffer.size;
						dstUniformBuffer.stageFlags = srcUniformBuffer.stageFlags;

						for (auto srcUniformId : srcUniformBuffer.uniformIds)
						{
							auto srcUniformData = srcLayout.GetUniformInfo(srcUniformId);

							DescriptorSetLayoutKey::UniformId dstUniformId = DescriptorSetLayoutKey::UniformId(
								res.uniformDatum.size());
							res.uniformDatum.push_back(DescriptorSetLayoutKey::UniformData());
							DescriptorSetLayoutKey::UniformData& dstUniformData = res.uniformDatum.back();

							dstUniformData.uniformBufferId = dstUniformBufferId;
							dstUniformData.offsetInBinding = srcUniformData.offsetInBinding;
							dstUniformData.size = srcUniformData.size;
							dstUniformData.name = srcUniformData.name;

							//memberData.
							dstUniformBuffer.uniformIds.push_back(dstUniformId);
						}
					}
					else
					{
						auto& dstUniformBuffer = res.uniformBufferDatum[dstUniformBufferId.id];
						dstUniformBuffer.stageFlags |= srcUniformBuffer.stageFlags;
						assert(srcUniformBuffer.shaderBindingIndex == dstUniformBuffer.shaderBindingIndex);
						assert(srcUniformBuffer.name == dstUniformBuffer.name);
						assert(srcUniformBuffer.size == dstUniformBuffer.size);
					}
				}
			}


			for (auto& imageSamplerBinding : imageSamplerBindings)
			{
				ImageSamplerId dstImageSamplerId;
				for (size_t setIndex = 0; setIndex < setsCount; setIndex++)
				{
					auto& srcLayout = setLayouts[setIndex];
					auto srcImageSamplerId = srcLayout.GetImageSamplerId(imageSamplerBinding);
					if (!srcImageSamplerId.IsValid()) continue;
					const auto& srcImageSampler = srcLayout.imageSamplerDatum[srcImageSamplerId.id];
					assert(srcImageSampler.shaderBindingIndex == imageSamplerBinding);

					if (!dstImageSamplerId.IsValid())
					{
						dstImageSamplerId = ImageSamplerId(res.imageSamplerDatum.size());
						res.imageSamplerDatum.push_back(ImageSamplerData());
						auto& dstImageSampler = res.imageSamplerDatum.back();

						dstImageSampler.shaderBindingIndex = srcImageSampler.shaderBindingIndex;
						dstImageSampler.name = srcImageSampler.name;
						dstImageSampler.stageFlags = srcImageSampler.stageFlags;
					}
					else
					{
						auto& dstImageSampler = res.imageSamplerDatum[dstImageSamplerId.id];
						dstImageSampler.stageFlags |= srcImageSampler.stageFlags;
						assert(srcImageSampler.shaderBindingIndex == dstImageSampler.shaderBindingIndex);
						assert(srcImageSampler.name == dstImageSampler.name);
					}
				}
			}
			for (auto& storageBufferBinding : storageBufferBindings)
			{
				StorageBufferId dstStorageBufferId;
				for (size_t setIndex = 0; setIndex < setsCount; setIndex++)
				{
					auto& srcLayout = setLayouts[setIndex];
					auto srcStorageBufferId = srcLayout.GetStorageBufferId(storageBufferBinding);
					if (!srcStorageBufferId.IsValid()) continue;
					const auto& srcStorageBuffer = srcLayout.storageBufferDatum[srcStorageBufferId.id];
					assert(srcStorageBuffer.shaderBindingIndex == storageBufferBinding);

					if (!dstStorageBufferId.IsValid())
					{
						dstStorageBufferId = StorageBufferId(res.storageBufferDatum.size());
						res.storageBufferDatum.push_back(StorageBufferData());
						auto& dstStorageBuffer = res.storageBufferDatum.back();

						dstStorageBuffer.shaderBindingIndex = srcStorageBuffer.shaderBindingIndex;
						dstStorageBuffer.name = srcStorageBuffer.name;
						dstStorageBuffer.arrayMemberSize = srcStorageBuffer.arrayMemberSize;
						dstStorageBuffer.podPartSize = srcStorageBuffer.podPartSize;
						dstStorageBuffer.stageFlags = srcStorageBuffer.stageFlags;
					}
					else
					{
						auto& dstStorageBuffer = res.storageBufferDatum[dstStorageBufferId.id];
						dstStorageBuffer.stageFlags |= srcStorageBuffer.stageFlags;
						assert(srcStorageBuffer.shaderBindingIndex == dstStorageBuffer.shaderBindingIndex);
						assert(srcStorageBuffer.name == dstStorageBuffer.name);
						assert(srcStorageBuffer.podPartSize == dstStorageBuffer.podPartSize);
						assert(srcStorageBuffer.arrayMemberSize == dstStorageBuffer.arrayMemberSize);
					}
				}
			}

			for (auto& storageImageBinding : storageImageBindings)
			{
				StorageImageId dstStorageImageId;
				for (size_t setIndex = 0; setIndex < setsCount; setIndex++)
				{
					auto& srcLayout = setLayouts[setIndex];
					auto srcStorageImageId = srcLayout.GetStorageImageId(storageImageBinding);
					if (!srcStorageImageId.IsValid()) continue;
					const auto& srcStorageImage = srcLayout.storageImageDatum[srcStorageImageId.id];
					assert(srcStorageImage.shaderBindingIndex == storageImageBinding);

					if (!dstStorageImageId.IsValid())
					{
						dstStorageImageId = StorageImageId(res.storageImageDatum.size());
						res.storageImageDatum.push_back(StorageImageData());
						auto& dstStorageImage = res.storageImageDatum.back();

						dstStorageImage.shaderBindingIndex = srcStorageImage.shaderBindingIndex;
						dstStorageImage.name = srcStorageImage.name;
						dstStorageImage.stageFlags = srcStorageImage.stageFlags;
					}
					else
					{
						auto& dstStorageImage = res.storageImageDatum[dstStorageImageId.id];
						dstStorageImage.stageFlags |= srcStorageImage.stageFlags;
						assert(srcStorageImage.shaderBindingIndex == dstStorageImage.shaderBindingIndex);
						assert(srcStorageImage.name == dstStorageImage.name);
					}
				}
			}


			res.RebuildIndex();
			return res;
		}

	private:
		void RebuildIndex()
		{
			uniformNameToIds.clear();
			for (size_t uniformIndex = 0; uniformIndex < uniformDatum.size(); ++uniformIndex)
			{
				UniformId uniformId = UniformId(uniformIndex);
				auto& uniformData = uniformDatum[uniformIndex];
				uniformNameToIds[uniformData.name] = uniformId;
			}

			uniformBufferNameToIds.clear();
			uniformBufferBindingToIds.clear();
			this->size = 0;
			for (size_t uniformBufferIndex = 0; uniformBufferIndex < uniformBufferDatum.size(); ++uniformBufferIndex)
			{
				UniformBufferId uniformBufferId = UniformBufferId(uniformBufferIndex);
				auto& uniformBufferData = uniformBufferDatum[uniformBufferIndex];
				uniformBufferNameToIds[uniformBufferData.name] = uniformBufferId;
				uniformBufferBindingToIds[uniformBufferData.shaderBindingIndex] = uniformBufferId;
				this->size += uniformBufferData.size;
			}

			imageSamplerNameToIds.clear();
			imageSamplerBindingToIds.clear();
			for (size_t imageSamplerIndex = 0; imageSamplerIndex < imageSamplerDatum.size(); imageSamplerIndex++)
			{
				ImageSamplerId imageSamplerId = ImageSamplerId(imageSamplerIndex);
				auto& imageSamplerData = imageSamplerDatum[imageSamplerIndex];
				imageSamplerNameToIds[imageSamplerData.name] = imageSamplerId;
				imageSamplerBindingToIds[imageSamplerData.shaderBindingIndex] = imageSamplerId;
			}

			storageBufferNameToIds.clear();
			storageBufferBindingToIds.clear();
			for (size_t storageBufferIndex = 0; storageBufferIndex < storageBufferDatum.size(); storageBufferIndex++)
			{
				StorageBufferId storageBufferId = StorageBufferId(storageBufferIndex);
				auto& storageBufferData = storageBufferDatum[storageBufferIndex];
				storageBufferNameToIds[storageBufferData.name] = storageBufferId;
				storageBufferBindingToIds[storageBufferData.shaderBindingIndex] = storageBufferId;
			}

			storageImageNameToIds.clear();
			storageImageBindingToIds.clear();
			for (size_t storageImageIndex = 0; storageImageIndex < storageImageDatum.size(); storageImageIndex++)
			{
				StorageImageId storageImageId = StorageImageId(storageImageIndex);
				auto& storageImageData = storageImageDatum[storageImageIndex];
				storageImageNameToIds[storageImageData.name] = storageImageId;
				storageImageBindingToIds[storageImageData.shaderBindingIndex] = storageImageId;
			}
		}


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
		Shader(vk::Device logicalDevice, std::string shaderFile)
		{
			auto bytecode = GetBytecode(shaderFile);
			/*vk::ShaderStageFlagBits shaderStage;
			if (shaderFile.find(".vert.spv") != std::string::npos)
			  shaderStage = vk::ShaderStageFlagBits::eVertex;
			if (shaderFile.find(".frag.spv") != std::string::npos)
			  shaderStage = vk::ShaderStageFlagBits::eFragment;
			if (shaderFile.find(".comp.spv") != std::string::npos)
			  shaderStage = vk::ShaderStageFlagBits::eCompute;*/
			Init(logicalDevice, bytecode);
		}

		Shader(vk::Device logicalDevice, const std::vector<uint32_t>& bytecode)
		{
			Init(logicalDevice, bytecode);
		}

		static const std::vector<uint32_t> GetBytecode(std::string filename)
		{
			std::ifstream file(filename, std::ios::ate | std::ios::binary);

			if (!file.is_open())
				throw std::runtime_error("failed to open file!");

			size_t fileSize = (size_t)file.tellg();
			std::vector<uint32_t> bytecode(fileSize / sizeof(uint32_t));

			file.seekg(0);
			file.read((char*)bytecode.data(), bytecode.size() * sizeof(uint32_t));
			file.close();
			return bytecode;
		}

		lz::ShaderModule* GetModule()
		{
			return shaderModule.get();
		}


		size_t GetSetsCount()
		{
			return descriptorSetLayoutKeys.size();
		}

		const DescriptorSetLayoutKey* GetSetInfo(size_t setIndex)
		{
			return &descriptorSetLayoutKeys[setIndex];
		}

		glm::uvec3 GetLocalSize()
		{
			return localSize;
		}

	private:
		void Init(vk::Device logicalDevice, const std::vector<uint32_t>& bytecode)
		{
			shaderModule.reset(new ShaderModule(logicalDevice, bytecode));
			localSize = glm::uvec3(0);
			spirv_cross::Compiler compiler(bytecode.data(), bytecode.size());

			std::vector<spirv_cross::EntryPoint> entryPoints = compiler.get_entry_points_and_stages();
			assert(entryPoints.size() == 1);
			vk::ShaderStageFlags stageFlags;
			switch (entryPoints[0].execution_model)
			{
			case spv::ExecutionModel::ExecutionModelVertex:
				{
					stageFlags |= vk::ShaderStageFlagBits::eVertex;
				}
				break;
			case spv::ExecutionModel::ExecutionModelFragment:
				{
					stageFlags |= vk::ShaderStageFlagBits::eFragment;
				}
				break;
			case spv::ExecutionModel::ExecutionModelGLCompute:
				{
					stageFlags |= vk::ShaderStageFlagBits::eCompute;
					localSize.x = compiler.get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, 0);
					localSize.y = compiler.get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, 1);
					localSize.z = compiler.get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, 2);
				}
				break;
			}

			spirv_cross::ShaderResources resources = compiler.get_shader_resources();


			struct SetResources
			{
				std::vector<spirv_cross::Resource> uniformBuffers;
				std::vector<spirv_cross::Resource> imageSamplers;
				std::vector<spirv_cross::Resource> storageBuffers;
				std::vector<spirv_cross::Resource> storageImages;
			};
			std::vector<SetResources> setResources;
			for (const auto& buffer : resources.uniform_buffers)
			{
				uint32_t setShaderId = compiler.get_decoration(buffer.id, spv::DecorationDescriptorSet);
				if (setShaderId >= setResources.size())
					setResources.resize(setShaderId + 1);
				setResources[setShaderId].uniformBuffers.push_back(buffer);
			}

			for (const auto& imageSampler : resources.sampled_images)
			{
				uint32_t setShaderId = compiler.get_decoration(imageSampler.id, spv::DecorationDescriptorSet);
				if (setShaderId >= setResources.size())
					setResources.resize(setShaderId + 1);
				setResources[setShaderId].imageSamplers.push_back(imageSampler);
			}

			for (const auto& buffer : resources.storage_buffers)
			{
				uint32_t setShaderId = compiler.get_decoration(buffer.id, spv::DecorationDescriptorSet);
				if (setShaderId >= setResources.size())
					setResources.resize(setShaderId + 1);
				setResources[setShaderId].storageBuffers.push_back(buffer);
			}

			for (const auto& image : resources.storage_images)
			{
				uint32_t setShaderId = compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
				if (setShaderId >= setResources.size())
					setResources.resize(setShaderId + 1);
				setResources[setShaderId].storageImages.push_back(image);
			}

			this->descriptorSetLayoutKeys.resize(setResources.size());
			for (size_t setIndex = 0; setIndex < setResources.size(); setIndex++)
			{
				auto& descriptorSetLayoutKey = descriptorSetLayoutKeys[setIndex];
				descriptorSetLayoutKey = DescriptorSetLayoutKey();
				descriptorSetLayoutKey.setShaderId = uint32_t(setIndex);

				for (auto buffer : setResources[setIndex].uniformBuffers)
				{
					uint32_t shaderBindingIndex = compiler.get_decoration(buffer.id, spv::DecorationBinding);

					auto bufferType = compiler.get_type(buffer.type_id);

					if (bufferType.basetype == spirv_cross::SPIRType::BaseType::Struct)
					{
						auto uniformBufferId = DescriptorSetLayoutKey::UniformBufferId(
							descriptorSetLayoutKey.uniformBufferDatum.size());
						descriptorSetLayoutKey.uniformBufferDatum.emplace_back(
							DescriptorSetLayoutKey::UniformBufferData());
						auto& bufferData = descriptorSetLayoutKey.uniformBufferDatum.back();

						bufferData.shaderBindingIndex = shaderBindingIndex;
						bufferData.name = buffer.name;
						bufferData.stageFlags = stageFlags;

						uint32_t declaredSize = uint32_t(compiler.get_declared_struct_size(bufferType));

						uint32_t currOffset = 0;
						for (uint32_t memberIndex = 0; memberIndex < bufferType.member_types.size(); memberIndex++)
						{
							uint32_t memberSize = uint32_t(
								compiler.get_declared_struct_member_size(bufferType, memberIndex));
							//auto memberType = compiler.get_type(bufferType.member_types[memberIndex]);
							auto memberName = compiler.get_member_name(buffer.base_type_id, memberIndex);

							//memberData.size = compiler.get_declared_struct_size(memeberType);
							DescriptorSetLayoutKey::UniformId uniformId = DescriptorSetLayoutKey::UniformId(
								descriptorSetLayoutKey.uniformDatum.size());
							descriptorSetLayoutKey.uniformDatum.push_back(DescriptorSetLayoutKey::UniformData());
							DescriptorSetLayoutKey::UniformData& uniformData = descriptorSetLayoutKey.uniformDatum.
								back();

							uniformData.uniformBufferId = uniformBufferId;
							uniformData.offsetInBinding = uint32_t(currOffset);
							uniformData.size = uint32_t(memberSize);
							uniformData.name = memberName;

							//memberData.
							bufferData.uniformIds.push_back(uniformId);

							currOffset += memberSize;
						}
						assert(currOffset == declaredSize);
						//alignment is wrong. avoid using smaller types before larger ones. completely avoid vec2/vec3
						bufferData.size = declaredSize;
						bufferData.offsetInSet = descriptorSetLayoutKey.size;

						descriptorSetLayoutKey.size += bufferData.size;
					}
				}

				for (auto imageSampler : setResources[setIndex].imageSamplers)
				{
					auto imageSamplerId = DescriptorSetLayoutKey::ImageSamplerId(
						descriptorSetLayoutKey.imageSamplerDatum.size());

					uint32_t shaderBindingIndex = compiler.get_decoration(imageSampler.id, spv::DecorationBinding);
					descriptorSetLayoutKey.imageSamplerDatum.push_back(DescriptorSetLayoutKey::ImageSamplerData());
					auto& imageSamplerData = descriptorSetLayoutKey.imageSamplerDatum.back();
					imageSamplerData.shaderBindingIndex = shaderBindingIndex;
					imageSamplerData.stageFlags = stageFlags;
					imageSamplerData.name = imageSampler.name;
				}

				for (auto buffer : setResources[setIndex].storageBuffers)
				{
					uint32_t shaderBindingIndex = compiler.get_decoration(buffer.id, spv::DecorationBinding);

					auto bufferType = compiler.get_type(buffer.type_id);

					if (bufferType.basetype == spirv_cross::SPIRType::BaseType::Struct)
					{
						auto storageBufferId = DescriptorSetLayoutKey::StorageBufferId(
							descriptorSetLayoutKey.storageBufferDatum.size());
						descriptorSetLayoutKey.storageBufferDatum.emplace_back(
							DescriptorSetLayoutKey::StorageBufferData());
						auto& bufferData = descriptorSetLayoutKey.storageBufferDatum.back();

						bufferData.shaderBindingIndex = shaderBindingIndex;
						bufferData.stageFlags = stageFlags;
						bufferData.name = buffer.name;
						bufferData.podPartSize = 0;
						bufferData.arrayMemberSize = 0;
						bufferData.offsetInSet = 0; //should not be used
						size_t declaredSize = compiler.get_declared_struct_size(bufferType);

						uint32_t currOffset = 0;
						bufferData.podPartSize = uint32_t(compiler.get_declared_struct_size(bufferType));

						//taken from get_declared_struct_size_runtime_array implementation
						auto& lastType = compiler.get_type(bufferType.member_types.back());
						if (!lastType.array.empty() && lastType.array_size_literal[0] && lastType.array[0] == 0)
							// Runtime array
							bufferData.arrayMemberSize = compiler.type_struct_member_array_stride(
								bufferType, uint32_t(bufferType.member_types.size() - 1));
						else
							bufferData.arrayMemberSize = 0;
					}
				}

				for (auto image : setResources[setIndex].storageImages)
				{
					auto imageSamplerId = DescriptorSetLayoutKey::ImageSamplerId(
						descriptorSetLayoutKey.imageSamplerDatum.size());

					uint32_t shaderBindingIndex = compiler.get_decoration(image.id, spv::DecorationBinding);
					descriptorSetLayoutKey.storageImageDatum.push_back(DescriptorSetLayoutKey::StorageImageData());
					auto& storageImageData = descriptorSetLayoutKey.storageImageDatum.back();
					storageImageData.shaderBindingIndex = shaderBindingIndex;
					storageImageData.stageFlags = stageFlags;
					storageImageData.name = image.name;
					//type?
				}
				descriptorSetLayoutKey.RebuildIndex();
			}
		}

		std::vector<DescriptorSetLayoutKey> descriptorSetLayoutKeys;

		std::unique_ptr<lz::ShaderModule> shaderModule;
		glm::uvec3 localSize;
	};

	class ShaderProgram
	{
	public:
		ShaderProgram(Shader* _vertexShader, Shader* _fragmentShader)
			: vertexShader(_vertexShader), fragmentShader(_fragmentShader)
		{
			combinedDescriptorSetLayoutKeys.resize(std::max(vertexShader->GetSetsCount(),
			                                                fragmentShader->GetSetsCount()));
			for (size_t setIndex = 0; setIndex < combinedDescriptorSetLayoutKeys.size(); setIndex++)
			{
				vk::DescriptorSetLayout setLayoutHandle = nullptr;
				std::vector<lz::DescriptorSetLayoutKey> setLayoutStageKeys;

				if (setIndex < vertexShader->GetSetsCount())
				{
					auto vertexSetInfo = vertexShader->GetSetInfo(setIndex);
					if (!vertexSetInfo->IsEmpty())
					{
						setLayoutStageKeys.push_back(*vertexSetInfo);
					}
				}
				if (setIndex < fragmentShader->GetSetsCount())
				{
					auto fragmentSetInfo = fragmentShader->GetSetInfo(setIndex);
					if (!fragmentSetInfo->IsEmpty())
					{
						setLayoutStageKeys.push_back(*fragmentSetInfo);
					}
				}
				this->combinedDescriptorSetLayoutKeys[setIndex] = DescriptorSetLayoutKey::Merge(
					setLayoutStageKeys.data(), setLayoutStageKeys.size());
			}
		}

		size_t GetSetsCount()
		{
			return combinedDescriptorSetLayoutKeys.size();
		}

		const DescriptorSetLayoutKey* GetSetInfo(size_t setIndex)
		{
			return &combinedDescriptorSetLayoutKeys[setIndex];
		}

		std::vector<DescriptorSetLayoutKey> combinedDescriptorSetLayoutKeys;
		Shader* vertexShader;
		Shader* fragmentShader;
	};
}
