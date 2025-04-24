#include "ShaderMemoryPool.h"

#include "ShaderProgram.h"

namespace lz
{
	ShaderMemoryPool::ShaderMemoryPool(uint32_t alignment)
	{
		this->alignment_ = alignment;
	}

	void ShaderMemoryPool::map_buffer(lz::Buffer* buffer)
	{
		this->buffer_ = buffer;
		dst_memory_ = this->buffer_->map();
		curr_offset_ = 0;
		curr_size_ = 0;
		curr_set_info_ = nullptr;
	}

	void ShaderMemoryPool::unmap_buffer()
	{
		buffer_->unmap();
		buffer_ = nullptr;
		dst_memory_ = nullptr;
	}

	lz::Buffer* ShaderMemoryPool::get_buffer() const
	{
		return buffer_;
	}

	ShaderMemoryPool::SetDynamicUniformBindings ShaderMemoryPool::begin_set(const lz::DescriptorSetLayoutKey* set_info)
	{
		this->curr_set_info_ = set_info;
		curr_size_ += curr_set_info_->get_total_constant_buffer_size();
		curr_size_ = align_size(curr_size_, alignment_);

		SetDynamicUniformBindings dynamic_bindings;
		dynamic_bindings.dynamic_offset = curr_offset_;

		std::vector<lz::DescriptorSetLayoutKey::UniformBufferId> uniform_buffer_ids;
		uniform_buffer_ids.resize(set_info->get_uniform_buffers_count());
		set_info->get_uniform_buffer_ids(uniform_buffer_ids.data());

		uint32_t set_uniform_total_size = 0;
		for (const auto uniform_buffer_id : uniform_buffer_ids)
		{
			const auto uniform_buffer_info = set_info->get_uniform_buffer_info(uniform_buffer_id);
			set_uniform_total_size += uniform_buffer_info.size;
			dynamic_bindings.uniform_buffer_bindings.push_back(lz::UniformBufferBinding(
				this->get_buffer(), uniform_buffer_info.shader_binding_index, uniform_buffer_info.offset_in_set,
				uniform_buffer_info.size));
		}
		assert(curr_set_info_->get_total_constant_buffer_size() == set_uniform_total_size);

		return dynamic_bindings;
	}

	void ShaderMemoryPool::end_set()
	{
		curr_offset_ = curr_size_;
		curr_set_info_ = nullptr;
	}

	
	uint32_t ShaderMemoryPool::align_size(uint32_t size, uint32_t alignment)
	{
		uint32_t res_size = size;
		if (res_size % alignment != 0)
		{
			res_size += (alignment - (res_size % alignment));
		}
		return res_size;
	}
}
