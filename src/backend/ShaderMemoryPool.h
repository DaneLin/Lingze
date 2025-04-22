#pragma once
namespace lz
{
	class ShaderMemoryPool
	{
	public:
		ShaderMemoryPool(uint32_t alignment)
		{
			this->alignment = alignment;
		}

		void MapBuffer(lz::Buffer* buffer)
		{
			this->buffer = buffer;
			dstMemory = this->buffer->Map();
			currOffset = 0;
			currSize = 0;
			currSetInfo = nullptr;
		}

		void UnmapBuffer()
		{
			buffer->Unmap();
			buffer = nullptr;
			dstMemory = nullptr;
		}

		lz::Buffer* GetBuffer()
		{
			return buffer;
		}

		struct SetDynamicUniformBindings
		{
			std::vector<UniformBufferBinding> uniformBufferBindings;
			uint32_t dynamicOffset;
		};

		SetDynamicUniformBindings BeginSet(const lz::DescriptorSetLayoutKey* setInfo)
		{
			this->currSetInfo = setInfo;
			currSize += currSetInfo->GetTotalConstantBufferSize();
			currSize = AlignSize(currSize, alignment);

			SetDynamicUniformBindings dynamicBindings;
			dynamicBindings.dynamicOffset = currOffset;

			std::vector<lz::DescriptorSetLayoutKey::UniformBufferId> uniformBufferIds;
			uniformBufferIds.resize(setInfo->GetUniformBuffersCount());
			setInfo->GetUniformBufferIds(uniformBufferIds.data());

			uint32_t setUniformTotalSize = 0;
			for (auto uniformBufferId : uniformBufferIds)
			{
				auto uniformBufferInfo = setInfo->GetUniformBufferInfo(uniformBufferId);
				setUniformTotalSize += uniformBufferInfo.size;
				dynamicBindings.uniformBufferBindings.push_back(lz::UniformBufferBinding(
					this->GetBuffer(), uniformBufferInfo.shaderBindingIndex, uniformBufferInfo.offsetInSet,
					uniformBufferInfo.size));
			}
			assert(currSetInfo->GetTotalConstantBufferSize() == setUniformTotalSize);

			return dynamicBindings;
		}

		void EndSet()
		{
			currOffset = currSize;
			currSetInfo = nullptr;
		}

		template <typename BufferType>
		BufferType* GetUniformBufferData(lz::DescriptorSetLayoutKey::UniformBufferId uniformBufferId)
		{
			auto bufferInfo = currSetInfo->GetUniformBufferInfo(uniformBufferId);
			size_t tmp = sizeof(BufferType);
			assert(bufferInfo.size == sizeof(BufferType));
			size_t totalOffset = currOffset + bufferInfo.offsetInSet;
			assert(totalOffset + sizeof(BufferType) <= currSize);
			return (BufferType*)((char*)dstMemory + totalOffset);
		}

		template <typename BufferType>
		BufferType* GetUniformBufferData(std::string bufferName)
		{
			auto bufferId = currSetInfo->GetUniformBufferId(bufferName);
			assert(!(bufferId == lz::DescriptorSetLayoutKey::UniformBufferId()));
			return GetUniformBufferData<BufferType>(bufferId);
		}

		template <typename UniformType>
		UniformType* GetUniformData(std::string uniformName)
		{
			auto uniformId = currSetInfo->GetUniformId(uniformName);
			return GetUniformData<UniformType>(uniformId);
		}

		template <typename UniformType>
		UniformType* GetUniformData(lz::DescriptorSetLayoutKey::UniformId uniformId)
		{
			auto uniformInfo = currSetInfo->GetUniformInfo(uniformId);
			auto bufferInfo = currSetInfo->GetUniformBufferInfo(uniformInfo.uniformBufferId);

			size_t totalOffset = currOffset + bufferInfo.offsetInSet + uniformInfo.offsetInBinding;
			assert(totalOffset + sizeof(UniformType) < currSize);
			return (UniformType*)((char*)dstMemory + totalOffset);
		}

	private:
		static uint32_t AlignSize(uint32_t size, uint32_t alignment)
		{
			uint32_t resSize = size;
			if (resSize % alignment != 0)
			{
				resSize += (alignment - (resSize % alignment));
			}
			return resSize;
		}

		uint32_t alignment;
		lz::Buffer* buffer;
		const lz::DescriptorSetLayoutKey* currSetInfo;
		void* dstMemory;
		uint32_t currOffset;
		uint32_t currSize;
	};
}
