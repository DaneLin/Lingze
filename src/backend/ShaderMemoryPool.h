#pragma once

#include "LingzeVK.h"
#include "ShaderProgram.h"

namespace lz
{
	class ShaderMemoryPool
	{
	public:
		ShaderMemoryPool(uint32_t alignment);

		void map_buffer(lz::Buffer* buffer);

		void unmap_buffer();

		lz::Buffer* get_buffer() const;

		struct SetDynamicUniformBindings
		{
			std::vector<UniformBufferBinding> uniform_buffer_bindings;
			uint32_t dynamic_offset;
		};

		SetDynamicUniformBindings begin_set(const lz::DescriptorSetLayoutKey* set_info);

		void end_set();

		template <typename BufferType>
		BufferType* get_uniform_buffer_data(lz::DescriptorSetLayoutKey::UniformBufferId uniform_buffer_id)
		{
			const auto buffer_info = curr_set_info_->get_uniform_buffer_info(uniform_buffer_id);
			size_t tmp = sizeof(BufferType);
			assert(buffer_info.size == sizeof(BufferType));
			const size_t total_offset = curr_offset_ + buffer_info.offset_in_set;
			assert(total_offset + sizeof(BufferType) <= curr_size_);
			return (BufferType*)((char*)dst_memory_ + total_offset);
		}

		template <typename BufferType>
		BufferType* get_uniform_buffer_data(std::string bufferName)
		{
			const auto buffer_id = curr_set_info_->get_uniform_buffer_id(bufferName);
			assert(!(buffer_id == lz::DescriptorSetLayoutKey::UniformBufferId()));
			return get_uniform_buffer_data<BufferType>(buffer_id);
		}

		template <typename UniformType>
		UniformType* get_uniform_data(std::string uniform_name)
		{
			auto uniformId = curr_set_info_->get_uniform_id(uniform_name);
			return get_uniform_data<UniformType>(uniformId);
		}

		template <typename UniformType>
		UniformType* get_uniform_data(lz::DescriptorSetLayoutKey::UniformId uniform_id)
		{
			const auto uniform_info = curr_set_info_->get_uniform_info(uniform_id);
			const auto buffer_info = curr_set_info_->get_uniform_buffer_info(uniform_info.uniform_buffer_id);

			const size_t total_offset = curr_offset_ + buffer_info.offset_in_set + uniform_info.offset_in_binding;
			assert(total_offset + sizeof(UniformType) < curr_size_);
			return (UniformType*)((char*)dst_memory_ + total_offset);
		}


	private:
		static uint32_t align_size(uint32_t size, uint32_t alignment);

		uint32_t alignment_;
		lz::Buffer* buffer_;
		const lz::DescriptorSetLayoutKey* curr_set_info_;
		void* dst_memory_;
		uint32_t curr_offset_;
		uint32_t curr_size_;
	};
}
