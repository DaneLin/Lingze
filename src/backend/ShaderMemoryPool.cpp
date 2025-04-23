#include "ShaderMemoryPool.h"

#include "ShaderProgram.h"

namespace lz
{
	ShaderMemoryPool::ShaderMemoryPool(uint32_t alignment)
	{
		this->alignment = alignment;
	}

	void ShaderMemoryPool::MapBuffer(lz::Buffer* buffer)
	{
		this->buffer = buffer;
		dstMemory = this->buffer->Map();
		currOffset = 0;
		currSize = 0;
		currSetInfo = nullptr;
	}

	void ShaderMemoryPool::UnmapBuffer()
	{
		buffer->Unmap();
		buffer = nullptr;
		dstMemory = nullptr;
	}

	lz::Buffer* ShaderMemoryPool::GetBuffer()
	{
		return buffer;
	}

	ShaderMemoryPool::SetDynamicUniformBindings ShaderMemoryPool::BeginSet(const lz::DescriptorSetLayoutKey* setInfo)
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

	void ShaderMemoryPool::EndSet()
	{
		currOffset = currSize;
		currSetInfo = nullptr;
	}

	
	uint32_t ShaderMemoryPool::AlignSize(uint32_t size, uint32_t alignment)
	{
		uint32_t resSize = size;
		if (resSize % alignment != 0)
		{
			resSize += (alignment - (resSize % alignment));
		}
		return resSize;
	}
}
