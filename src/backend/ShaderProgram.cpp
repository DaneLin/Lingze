#include "ShaderProgram.h"

#include "ShaderModule.h"

// Include glslang headers
#include <iostream>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>

namespace lz
{

	class DirStackFileIncluder : public glslang::TShader::Includer {
	public:
		DirStackFileIncluder() : externalLocalDirectoryCount(0) { }

		virtual IncludeResult* includeLocal(const char* headerName,
			const char* includerName,
			size_t inclusionDepth) override
		{
			return readLocalPath(headerName, includerName, (int)inclusionDepth);
		}

		virtual IncludeResult* includeSystem(const char* headerName,
			const char* /*includerName*/,
			size_t /*inclusionDepth*/) override
		{
			return readSystemPath(headerName);
		}

		// Externally set directories. E.g., from a command-line -I<dir>.
		//  - Most-recently pushed are checked first.
		//  - All these are checked after the parse-time stack of local directories
		//    is checked.
		//  - This only applies to the "local" form of #include.
		//  - Makes its own copy of the path.
		virtual void pushExternalLocalDirectory(const std::string& dir)
		{
			directoryStack.push_back(dir);
			externalLocalDirectoryCount = (int)directoryStack.size();
		}

		virtual void releaseInclude(IncludeResult* result) override
		{
			if (result != nullptr) {
				delete[] static_cast<tUserDataElement*>(result->userData);
				delete result;
			}
		}

		virtual std::set<std::string> getIncludedFiles()
		{
			return includedFiles;
		}

		virtual ~DirStackFileIncluder() override { }

	protected:
		typedef char tUserDataElement;
		std::vector<std::string> directoryStack;
		int externalLocalDirectoryCount;
		std::set<std::string> includedFiles;

		// Search for a valid "local" path based on combining the stack of include
		// directories and the nominal name of the header.
		virtual IncludeResult* readLocalPath(const char* headerName, const char* includerName, int depth)
		{
			// Discard popped include directories, and
			// initialize when at parse-time first level.
			directoryStack.resize(depth + externalLocalDirectoryCount);
			if (depth == 1)
				directoryStack.back() = getDirectory(includerName);

			// Find a directory that works, using a reverse search of the include stack.
			for (auto it = directoryStack.rbegin(); it != directoryStack.rend(); ++it) {
				std::string path = *it + '/' + headerName;
				std::replace(path.begin(), path.end(), '\\', '/');
				std::ifstream file(path, std::ios_base::binary | std::ios_base::ate);
				if (file) {
					directoryStack.push_back(getDirectory(path));
					includedFiles.insert(path);
					return newIncludeResult(path, file, (int)file.tellg());
				}
			}

			return nullptr;
		}

		// Search for a valid <system> path.
		// Not implemented yet; returning nullptr signals failure to find.
		virtual IncludeResult* readSystemPath(const char* /*headerName*/) const
		{
			return nullptr;
		}

		// Do actual reading of the file, filling in a new include result.
		virtual IncludeResult* newIncludeResult(const std::string& path, std::ifstream& file, int length) const
		{
			char* content = new tUserDataElement[length];
			file.seekg(0, file.beg);
			file.read(content, length);
			return new IncludeResult(path, content, length, content);
		}

		// If no path markers, return current working directory.
		// Otherwise, strip file name and return path leading up to it.
		virtual std::string getDirectory(const std::string path) const
		{
			size_t last = path.find_last_of("/\\");
			return last == std::string::npos ? "." : path.substr(0, last);
		}
	};

	ImageSamplerBinding::ImageSamplerBinding(): image_view(nullptr), sampler(nullptr)
	{
	}

	ImageSamplerBinding::ImageSamplerBinding(lz::ImageView* image_view, lz::Sampler* sampler,
	                                         uint32_t shader_binding_id): image_view(image_view), sampler(sampler),
	                                                                      shader_binding_id(shader_binding_id)
	{
		assert(image_view);
		assert(sampler);
	}

	bool ImageSamplerBinding::operator<(const ImageSamplerBinding& other) const
	{
		return std::tie(image_view, sampler, shader_binding_id) < std::tie(
			other.image_view, other.sampler, other.shader_binding_id);
	}

	UniformBufferBinding::UniformBufferBinding(): buffer(nullptr), offset(-1), size(-1)
	{
	}

	UniformBufferBinding::UniformBufferBinding(lz::Buffer* buffer, uint32_t shader_binding_id, vk::DeviceSize offset,
	                                           vk::DeviceSize size): buffer(buffer),
	                                                                 shader_binding_id(shader_binding_id),
	                                                                 offset(offset), size(size)
	{
		assert(buffer);
	}

	bool UniformBufferBinding::operator<(const UniformBufferBinding& other) const
	{
		return std::tie(buffer, shader_binding_id, offset, size) < std::tie(
			other.buffer, other.shader_binding_id, other.offset, other.size);
	}

	StorageBufferBinding::StorageBufferBinding(): buffer(nullptr), offset(-1), size(-1)
	{
	}

	StorageBufferBinding::StorageBufferBinding(lz::Buffer* buffer, uint32_t shader_binding_id, vk::DeviceSize offset,
	                                           vk::DeviceSize size): buffer(buffer),
	                                                                 shader_binding_id(shader_binding_id),
	                                                                 offset(offset), size(size)
	{
		assert(buffer);
	}

	bool StorageBufferBinding::operator<(const StorageBufferBinding& other) const
	{
		return std::tie(buffer, shader_binding_id, offset, size) < std::tie(
			other.buffer, other.shader_binding_id, other.offset, other.size);
	}

	StorageImageBinding::StorageImageBinding(): image_view(nullptr)
	{
	}

	StorageImageBinding::StorageImageBinding(lz::ImageView* image_view, uint32_t shader_binding_id):
		image_view(image_view),
		shader_binding_id(shader_binding_id)
	{
		assert(image_view);
	}

	bool StorageImageBinding::operator<(const StorageImageBinding& other) const
	{
		return std::tie(image_view, shader_binding_id) < std::tie(other.image_view, other.shader_binding_id);
	}

	bool DescriptorSetLayoutKey::UniformData::operator<(const UniformData& other) const
	{
		return std::tie(name, offset_in_binding, size) < std::tie(other.name, other.offset_in_binding, other.size);
	}

	bool DescriptorSetLayoutKey::UniformBufferData::operator<(const UniformBufferData& other) const
	{
		return std::tie(name, shader_binding_index, size) < std::tie(
			other.name, other.shader_binding_index, other.size);
	}

	bool DescriptorSetLayoutKey::ImageSamplerData::operator<(const ImageSamplerData& other) const
	{
		return std::tie(name, shader_binding_index) < std::tie(other.name, other.shader_binding_index);
	}

	bool DescriptorSetLayoutKey::StorageBufferData::operator<(const StorageBufferData& other) const
	{
		return std::tie(name, shader_binding_index, pod_part_size, array_member_size) < std::tie(
			other.name, other.shader_binding_index, other.pod_part_size, other.array_member_size);
	}

	bool DescriptorSetLayoutKey::StorageImageData::operator<(const StorageImageData& other) const
	{
		return std::tie(name, shader_binding_index) < std::tie(other.name, other.shader_binding_index);
	}

	size_t DescriptorSetLayoutKey::get_uniform_buffers_count() const
	{
		return uniform_buffer_datum_.size();
	}

	void DescriptorSetLayoutKey::get_uniform_buffer_ids(UniformBufferId* dst_uniform_buffer_ids, size_t count,
	                                                    size_t offset) const
	{
		if (count == -1)
		{
			count = uniform_buffer_datum_.size();
		}
		assert(count + offset <= uniform_buffer_datum_.size());
		for (size_t index = offset; index < offset + count; ++index)
		{
			dst_uniform_buffer_ids[index] = UniformBufferId(index);
		}
	}

	DescriptorSetLayoutKey::UniformBufferId DescriptorSetLayoutKey::get_uniform_buffer_id(std::string buffer_name) const
	{
		auto it = uniform_buffer_name_to_ids_.find(buffer_name);
		if (it == uniform_buffer_name_to_ids_.end())
		{
			return UniformBufferId();
		}
		return it->second;
	}

	DescriptorSetLayoutKey::UniformBufferId DescriptorSetLayoutKey::get_uniform_buffer_id(
		uint32_t buffer_binding_id) const
	{
		auto it = uniform_buffer_binding_to_ids_.find(buffer_binding_id);
		if (it == uniform_buffer_binding_to_ids_.end())
		{
			return UniformBufferId();
		}
		return it->second;
	}

	DescriptorSetLayoutKey::UniformBufferData DescriptorSetLayoutKey::get_uniform_buffer_info(
		UniformBufferId uniform_buffer_id) const
	{
		return uniform_buffer_datum_[uniform_buffer_id.id_];
	}

	UniformBufferBinding DescriptorSetLayoutKey::make_uniform_buffer_binding(
		std::string buffer_name, lz::Buffer* buffer,
		vk::DeviceSize offset, vk::DeviceSize size) const
	{
		const auto uniform_buffer_id = get_uniform_buffer_id(buffer_name);
		assert(uniform_buffer_id.is_valid());
		const auto uniform_buffer_info = get_uniform_buffer_info(uniform_buffer_id);
		return UniformBufferBinding(buffer, uniform_buffer_info.shader_binding_index, offset, size);
	}

	size_t DescriptorSetLayoutKey::get_storage_buffers_count() const
	{
		return storage_buffer_datum_.size();
	}

	void DescriptorSetLayoutKey::get_storage_buffer_ids(StorageBufferId* dst_storage_buffer_ids, size_t count,
	                                                    size_t offset) const
	{
		if (count == -1)
			count = storage_buffer_datum_.size();
		assert(count + offset <= storage_buffer_datum_.size());
		for (size_t index = offset; index < offset + count; index++)
			dst_storage_buffer_ids[index] = StorageBufferId(index);
	}

	DescriptorSetLayoutKey::StorageBufferId DescriptorSetLayoutKey::get_storage_buffer_id(
		std::string storage_buffer_name) const
	{
		auto it = storage_buffer_name_to_ids_.find(storage_buffer_name);
		if (it == storage_buffer_name_to_ids_.end())
			return StorageBufferId();
		return it->second;
	}

	DescriptorSetLayoutKey::StorageBufferId DescriptorSetLayoutKey::get_storage_buffer_id(
		uint32_t buffer_binding_id) const
	{
		auto it = storage_buffer_binding_to_ids_.find(buffer_binding_id);
		if (it == storage_buffer_binding_to_ids_.end())
			return StorageBufferId();
		return it->second;
	}

	DescriptorSetLayoutKey::StorageBufferData DescriptorSetLayoutKey::get_storage_buffer_info(
		StorageBufferId storage_buffer_id) const
	{
		return storage_buffer_datum_[storage_buffer_id.id_];
	}

	StorageBufferBinding DescriptorSetLayoutKey::make_storage_buffer_binding(
		std::string buffer_name, lz::Buffer* buffer,
		vk::DeviceSize offset, vk::DeviceSize size) const
	{
		const auto storage_buffer_id = get_storage_buffer_id(buffer_name);
		assert(storage_buffer_id.is_valid());
		const auto storage_buffer_info = get_storage_buffer_info(storage_buffer_id);
		return StorageBufferBinding(buffer, storage_buffer_info.shader_binding_index, offset, size);
	}


	size_t DescriptorSetLayoutKey::get_uniforms_count() const
	{
		return uniform_datum_.size();
	}

	void DescriptorSetLayoutKey::get_uniform_ids(UniformId* dst_uniform_ids, size_t count, size_t offset) const
	{
		if (count == -1)
		{
			count = uniform_datum_.size();
		}
		assert(count + offset <= uniform_datum_.size());
		for (size_t index = offset; index < offset + count; ++index)
		{
			dst_uniform_ids[index] = UniformId(index);
		}
	}

	DescriptorSetLayoutKey::UniformData DescriptorSetLayoutKey::get_uniform_info(UniformId uniform_id) const
	{
		return uniform_datum_[uniform_id.id_];
	}

	DescriptorSetLayoutKey::UniformId DescriptorSetLayoutKey::get_uniform_id(std::string name) const
	{
		const auto it = uniform_name_to_ids_.find(name);
		if (it == uniform_name_to_ids_.end())
			return UniformId();
		return it->second;
	}

	size_t DescriptorSetLayoutKey::get_image_samplers_count() const
	{
		return image_sampler_datum_.size();
	}

	void DescriptorSetLayoutKey::get_image_sampler_ids(ImageSamplerId* dst_image_sampler_ids, size_t count,
	                                                   size_t offset) const
	{
		if (count == -1)
			count = image_sampler_datum_.size();
		assert(count + offset <= image_sampler_datum_.size());
		for (size_t index = offset; index < offset + count; index++)
			dst_image_sampler_ids[index] = ImageSamplerId(index);
	}

	DescriptorSetLayoutKey::ImageSamplerData DescriptorSetLayoutKey::get_image_sampler_info(
		ImageSamplerId image_sampler_id) const
	{
		return image_sampler_datum_[image_sampler_id.id_];
	}

	DescriptorSetLayoutKey::ImageSamplerId DescriptorSetLayoutKey::get_image_sampler_id(
		std::string image_sampler_name) const
	{
		auto it = image_sampler_name_to_ids_.find(image_sampler_name);
		if (it == image_sampler_name_to_ids_.end())
			return ImageSamplerId();
		return it->second;
	}

	DescriptorSetLayoutKey::ImageSamplerId DescriptorSetLayoutKey::get_image_sampler_id(
		uint32_t shader_binding_index) const
	{
		auto it = image_sampler_binding_to_ids_.find(shader_binding_index);
		if (it == image_sampler_binding_to_ids_.end())
			return ImageSamplerId();
		return it->second;
	}

	ImageSamplerBinding DescriptorSetLayoutKey::make_image_sampler_binding(std::string image_sampler_name,
	                                                                       lz::ImageView* image_view,
	                                                                       lz::Sampler* sampler) const
	{
		const auto image_sampler_id = get_image_sampler_id(image_sampler_name);
		assert(image_sampler_id.is_valid());
		const auto image_sampler_info = get_image_sampler_info(image_sampler_id);
		return ImageSamplerBinding(image_view, sampler, image_sampler_info.shader_binding_index);
	}

	size_t DescriptorSetLayoutKey::get_storage_images_count() const
	{
		return storage_image_datum_.size();
	}

	void DescriptorSetLayoutKey::get_storage_image_ids(StorageImageId* dst_storage_image_ids, size_t count,
	                                                   size_t offset) const
	{
		if (count == -1)
			count = storage_image_datum_.size();
		assert(count + offset <= storage_image_datum_.size());
		for (size_t index = offset; index < offset + count; index++)
			dst_storage_image_ids[index] = StorageImageId(index);
	}

	DescriptorSetLayoutKey::StorageImageId DescriptorSetLayoutKey::get_storage_image_id(
		std::string storage_image_name) const
	{
		auto it = storage_image_name_to_ids_.find(storage_image_name);
		if (it == storage_image_name_to_ids_.end())
			return StorageImageId();
		return it->second;
	}

	DescriptorSetLayoutKey::StorageImageId DescriptorSetLayoutKey::get_storage_image_id(
		uint32_t buffer_binding_id) const
	{
		auto it = storage_image_binding_to_ids_.find(buffer_binding_id);
		if (it == storage_image_binding_to_ids_.end())
			return StorageImageId();
		return it->second;
	}

	DescriptorSetLayoutKey::StorageImageData DescriptorSetLayoutKey::get_storage_image_info(
		StorageImageId storage_image_id) const
	{
		return storage_image_datum_[storage_image_id.id_];
	}

	StorageImageBinding DescriptorSetLayoutKey::make_storage_image_binding(std::string image_name,
	                                                                       lz::ImageView* image_view) const
	{
		const auto image_id = get_storage_image_id(image_name);
		assert(image_id.is_valid());
		const auto storage_image_info = get_storage_image_info(image_id);
		return StorageImageBinding(image_view, storage_image_info.shader_binding_index);
	}

	uint32_t DescriptorSetLayoutKey::get_total_constant_buffer_size() const
	{
		return size_;
	}

	bool DescriptorSetLayoutKey::is_empty() const
	{
		return get_image_samplers_count() == 0 && get_uniform_buffers_count() == 0 && get_storage_images_count() == 0 &&
			get_storage_buffers_count() == 0;
	}

	bool DescriptorSetLayoutKey::operator<(const DescriptorSetLayoutKey& other) const
	{
		return std::tie(uniform_datum_, uniform_buffer_datum_, image_sampler_datum_, storage_buffer_datum_,
		                storage_image_datum_)
			< std::tie(other.uniform_datum_, other.uniform_buffer_datum_, other.image_sampler_datum_,
			           other.storage_buffer_datum_, other.storage_image_datum_);
	}

	DescriptorSetLayoutKey DescriptorSetLayoutKey::merge(DescriptorSetLayoutKey* set_layouts, size_t sets_count)
	{
		DescriptorSetLayoutKey res;
		if (sets_count <= 0) return res;

		res.set_shader_id_ = set_layouts[0].set_shader_id_;
		for (size_t layout_index = 0; layout_index < sets_count; layout_index++)
			assert(set_layouts[layout_index].set_shader_id_ == res.set_shader_id_);

		std::set<uint32_t> uniform_buffer_bindings;
		std::set<uint32_t> image_sampler_bindings;
		std::set<uint32_t> storage_buffer_bindings;
		std::set<uint32_t> storage_image_bindings;

		for (size_t set_index = 0; set_index < sets_count; set_index++)
		{
			auto& set_layout = set_layouts[set_index];
			for (auto& uniform_buffer_data : set_layout.uniform_buffer_datum_)
			{
				uniform_buffer_bindings.insert(uniform_buffer_data.shader_binding_index);
			}
			for (auto& image_sampler_data : set_layout.image_sampler_datum_)
			{
				image_sampler_bindings.insert(image_sampler_data.shader_binding_index);
			}
			for (auto& storage_buffer_data : set_layout.storage_buffer_datum_)
			{
				storage_buffer_bindings.insert(storage_buffer_data.shader_binding_index);
			}
			for (auto& storage_image_data : set_layout.storage_image_datum_)
			{
				storage_image_bindings.insert(storage_image_data.shader_binding_index);
			}
		}

		for (auto& uniform_buffer_binding : uniform_buffer_bindings)
		{
			UniformBufferId dst_uniform_buffer_id;
			for (size_t set_index = 0; set_index < sets_count; set_index++)
			{
				auto& src_layout = set_layouts[set_index];
				auto src_uniform_buffer_id = src_layout.get_uniform_buffer_id(uniform_buffer_binding);
				if (!src_uniform_buffer_id.is_valid()) continue;
				const auto& src_uniform_buffer = src_layout.uniform_buffer_datum_[src_uniform_buffer_id.id_];
				assert(src_uniform_buffer.shader_binding_index == uniform_buffer_binding);

				if (!dst_uniform_buffer_id.is_valid())
				{
					dst_uniform_buffer_id = UniformBufferId(res.uniform_buffer_datum_.size());
					res.uniform_buffer_datum_.push_back(UniformBufferData());
					auto& dst_uniform_buffer = res.uniform_buffer_datum_.back();

					dst_uniform_buffer.shader_binding_index = src_uniform_buffer.shader_binding_index;
					dst_uniform_buffer.name = src_uniform_buffer.name;
					dst_uniform_buffer.size = src_uniform_buffer.size;
					dst_uniform_buffer.stage_flags = src_uniform_buffer.stage_flags;

					for (auto src_uniform_id : src_uniform_buffer.uniform_ids)
					{
						auto src_uniform_data = src_layout.get_uniform_info(src_uniform_id);

						DescriptorSetLayoutKey::UniformId dst_uniform_id = DescriptorSetLayoutKey::UniformId(
							res.uniform_datum_.size());
						res.uniform_datum_.push_back(DescriptorSetLayoutKey::UniformData());
						DescriptorSetLayoutKey::UniformData& dst_uniform_data = res.uniform_datum_.back();

						dst_uniform_data.uniform_buffer_id = dst_uniform_buffer_id;
						dst_uniform_data.offset_in_binding = src_uniform_data.offset_in_binding;
						dst_uniform_data.size = src_uniform_data.size;
						dst_uniform_data.name = src_uniform_data.name;

						//memberData.
						dst_uniform_buffer.uniform_ids.push_back(dst_uniform_id);
					}
				}
				else
				{
					auto& dst_uniform_buffer = res.uniform_buffer_datum_[dst_uniform_buffer_id.id_];
					dst_uniform_buffer.stage_flags |= src_uniform_buffer.stage_flags;
					assert(src_uniform_buffer.shader_binding_index == dst_uniform_buffer.shader_binding_index);
					assert(src_uniform_buffer.name == dst_uniform_buffer.name);
					assert(src_uniform_buffer.size == dst_uniform_buffer.size);
				}
			}
		}


		for (auto& image_sampler_binding : image_sampler_bindings)
		{
			ImageSamplerId dst_image_sampler_id;
			for (size_t set_index = 0; set_index < sets_count; ++set_index)
			{
				auto& src_layout = set_layouts[set_index];
				auto src_image_sampler_id = src_layout.get_image_sampler_id(image_sampler_binding);
				if (!src_image_sampler_id.is_valid()) continue;
				const auto& src_image_sampler = src_layout.image_sampler_datum_[src_image_sampler_id.id_];
				assert(src_image_sampler.shader_binding_index == image_sampler_binding);

				if (!dst_image_sampler_id.is_valid())
				{
					dst_image_sampler_id = ImageSamplerId(res.image_sampler_datum_.size());
					res.image_sampler_datum_.push_back(ImageSamplerData());
					auto& dst_image_sampler = res.image_sampler_datum_.back();

					dst_image_sampler.shader_binding_index = src_image_sampler.shader_binding_index;
					dst_image_sampler.name = src_image_sampler.name;
					dst_image_sampler.stage_flags = src_image_sampler.stage_flags;
				}
				else
				{
					auto& dst_image_sampler = res.image_sampler_datum_[dst_image_sampler_id.id_];
					dst_image_sampler.stage_flags |= src_image_sampler.stage_flags;
					assert(src_image_sampler.shader_binding_index == dst_image_sampler.shader_binding_index);
					assert(src_image_sampler.name == dst_image_sampler.name);
				}
			}
		}
		for (auto& storage_buffer_binding : storage_buffer_bindings)
		{
			StorageBufferId dst_storage_buffer_id;
			for (size_t set_index = 0; set_index < sets_count; ++set_index)
			{
				auto& src_layout = set_layouts[set_index];
				auto src_storage_buffer_id = src_layout.get_storage_buffer_id(storage_buffer_binding);
				if (!src_storage_buffer_id.is_valid()) continue;
				const auto& src_storage_buffer = src_layout.storage_buffer_datum_[src_storage_buffer_id.id_];
				assert(src_storage_buffer.shader_binding_index == storage_buffer_binding);

				if (!dst_storage_buffer_id.is_valid())
				{
					dst_storage_buffer_id = StorageBufferId(res.storage_buffer_datum_.size());
					res.storage_buffer_datum_.push_back(StorageBufferData());
					auto& dst_storage_buffer = res.storage_buffer_datum_.back();

					dst_storage_buffer.shader_binding_index = src_storage_buffer.shader_binding_index;
					dst_storage_buffer.name = src_storage_buffer.name;
					dst_storage_buffer.array_member_size = src_storage_buffer.array_member_size;
					dst_storage_buffer.pod_part_size = src_storage_buffer.pod_part_size;
					dst_storage_buffer.stage_flags = src_storage_buffer.stage_flags;
				}
				else
				{
					auto& dst_storage_buffer = res.storage_buffer_datum_[dst_storage_buffer_id.id_];
					dst_storage_buffer.stage_flags |= src_storage_buffer.stage_flags;
					assert(src_storage_buffer.shader_binding_index == dst_storage_buffer.shader_binding_index);
					assert(src_storage_buffer.name == dst_storage_buffer.name);
					assert(src_storage_buffer.pod_part_size == dst_storage_buffer.pod_part_size);
					assert(src_storage_buffer.array_member_size == dst_storage_buffer.array_member_size);
				}
			}
		}

		for (auto& storage_image_binding : storage_image_bindings)
		{
			StorageImageId dst_storage_image_id;
			for (size_t set_index = 0; set_index < sets_count; set_index++)
			{
				auto& src_layout = set_layouts[set_index];
				auto src_storage_image_id = src_layout.get_storage_image_id(storage_image_binding);
				if (!src_storage_image_id.is_valid()) continue;
				const auto& src_storage_image = src_layout.storage_image_datum_[src_storage_image_id.id_];
				assert(src_storage_image.shader_binding_index == storage_image_binding);

				if (!dst_storage_image_id.is_valid())
				{
					dst_storage_image_id = StorageImageId(res.storage_image_datum_.size());
					res.storage_image_datum_.push_back(StorageImageData());
					auto& dst_storage_image = res.storage_image_datum_.back();

					dst_storage_image.shader_binding_index = src_storage_image.shader_binding_index;
					dst_storage_image.name = src_storage_image.name;
					dst_storage_image.stage_flags = src_storage_image.stage_flags;
				}
				else
				{
					auto& dst_storage_image = res.storage_image_datum_[dst_storage_image_id.id_];
					dst_storage_image.stage_flags |= src_storage_image.stage_flags;
					assert(src_storage_image.shader_binding_index == dst_storage_image.shader_binding_index);
					assert(src_storage_image.name == dst_storage_image.name);
				}
			}
		}

		res.rebuild_index();
		return res;
	}

	void DescriptorSetLayoutKey::rebuild_index()
	{
		uniform_name_to_ids_.clear();
		for (size_t uniform_index = 0; uniform_index < uniform_datum_.size(); ++uniform_index)
		{
			const UniformId uniform_id = UniformId(uniform_index);
			auto& uniform_data = uniform_datum_[uniform_index];
			uniform_name_to_ids_[uniform_data.name] = uniform_id;
		}

		uniform_buffer_name_to_ids_.clear();
		uniform_buffer_binding_to_ids_.clear();
		this->size_ = 0;
		for (size_t uniform_buffer_index = 0; uniform_buffer_index < uniform_buffer_datum_.size(); ++uniform_buffer_index)
		{
			const UniformBufferId uniform_buffer_id = UniformBufferId(uniform_buffer_index);
			auto& uniform_buffer_data = uniform_buffer_datum_[uniform_buffer_index];
			uniform_buffer_name_to_ids_[uniform_buffer_data.name] = uniform_buffer_id;
			uniform_buffer_binding_to_ids_[uniform_buffer_data.shader_binding_index] = uniform_buffer_id;
			this->size_ += uniform_buffer_data.size;
		}

		image_sampler_name_to_ids_.clear();
		image_sampler_binding_to_ids_.clear();
		for (size_t image_sampler_index = 0; image_sampler_index < image_sampler_datum_.size(); ++image_sampler_index)
		{
			const ImageSamplerId image_sampler_id = ImageSamplerId(image_sampler_index);
			auto& image_sampler_data = image_sampler_datum_[image_sampler_index];
			image_sampler_name_to_ids_[image_sampler_data.name] = image_sampler_id;
			image_sampler_binding_to_ids_[image_sampler_data.shader_binding_index] = image_sampler_id;
		}

		storage_buffer_name_to_ids_.clear();
		storage_buffer_binding_to_ids_.clear();
		for (size_t storage_buffer_index = 0; storage_buffer_index < storage_buffer_datum_.size(); ++storage_buffer_index)
		{
			const StorageBufferId storage_buffer_id = StorageBufferId(storage_buffer_index);
			auto& storage_buffer_data = storage_buffer_datum_[storage_buffer_index];
			storage_buffer_name_to_ids_[storage_buffer_data.name] = storage_buffer_id;
			storage_buffer_binding_to_ids_[storage_buffer_data.shader_binding_index] = storage_buffer_id;
		}

		storage_image_name_to_ids_.clear();
		storage_image_binding_to_ids_.clear();
		for (size_t storage_image_index = 0; storage_image_index < storage_image_datum_.size(); ++storage_image_index)
		{
			const StorageImageId storage_image_id = StorageImageId(storage_image_index);
			auto& storage_image_data = storage_image_datum_[storage_image_index];
			storage_image_name_to_ids_[storage_image_data.name] = storage_image_id;
			storage_image_binding_to_ids_[storage_image_data.shader_binding_index] = storage_image_id;
		}
	}

	// Helper function to get shader stage from filename extension
	EShLanguage findLanguageFromExtension(const std::string& filename) {
		std::string ext = filename.substr(filename.find_last_of('.') + 1);
		if (ext == "vert") return EShLangVertex;
		if (ext == "frag") return EShLangFragment;
		if (ext == "comp") return EShLangCompute;
		if (ext == "tesc") return EShLangTessControl;
		if (ext == "tese") return EShLangTessEvaluation;
		if (ext == "geom") return EShLangGeometry;
		if (ext == "mesh") return EShLangMeshNV;
		if (ext == "task") return EShLangTaskNV;
		return EShLangVertex;
	}

	// Helper function to read shader source code
	std::string readShaderFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open()) {
			throw std::runtime_error("failed to open file: " + filename);
		}

		size_t fileSize = static_cast<size_t>(file.tellg());
		std::string buffer;
		buffer.resize(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}

	// Initialize glslang (call once)
	static bool glslangInitialized = false;
	static void initializeGlslang() {
		if (!glslangInitialized) {
			glslang::InitializeProcess();
			glslangInitialized = true;
		}
	}

	// Finalize glslang (call on app exit)
	class GlslangFinalizer {
	public:
		~GlslangFinalizer() {
			if (glslangInitialized) {
				glslang::FinalizeProcess();
				glslangInitialized = false;
			}
		}
	};
	static GlslangFinalizer glslangFinalizer;

	// Get default resource limits
	const TBuiltInResource* getDefaultResources() {
		static TBuiltInResource DefaultTBuiltInResource = {
		/* .MaxLights = */ 32,
    /* .MaxClipPlanes = */ 6,
    /* .MaxTextureUnits = */ 32,
    /* .MaxTextureCoords = */ 32,
    /* .MaxVertexAttribs = */ 64,
    /* .MaxVertexUniformComponents = */ 4096,
    /* .MaxVaryingFloats = */ 64,
    /* .MaxVertexTextureImageUnits = */ 32,
    /* .MaxCombinedTextureImageUnits = */ 80,
    /* .MaxTextureImageUnits = */ 32,
    /* .MaxFragmentUniformComponents = */ 4096,
    /* .MaxDrawBuffers = */ 32,
    /* .MaxVertexUniformVectors = */ 128,
    /* .MaxVaryingVectors = */ 8,
    /* .MaxFragmentUniformVectors = */ 16,
    /* .MaxVertexOutputVectors = */ 16,
    /* .MaxFragmentInputVectors = */ 15,
    /* .MinProgramTexelOffset = */ -8,
    /* .MaxProgramTexelOffset = */ 7,
    /* .MaxClipDistances = */ 8,
    /* .MaxComputeWorkGroupCountX = */ 65535,
    /* .MaxComputeWorkGroupCountY = */ 65535,
    /* .MaxComputeWorkGroupCountZ = */ 65535,
    /* .MaxComputeWorkGroupSizeX = */ 1024,
    /* .MaxComputeWorkGroupSizeY = */ 1024,
    /* .MaxComputeWorkGroupSizeZ = */ 64,
    /* .MaxComputeUniformComponents = */ 1024,
    /* .MaxComputeTextureImageUnits = */ 16,
    /* .MaxComputeImageUniforms = */ 8,
    /* .MaxComputeAtomicCounters = */ 8,
    /* .MaxComputeAtomicCounterBuffers = */ 1,
    /* .MaxVaryingComponents = */ 60,
    /* .MaxVertexOutputComponents = */ 64,
    /* .MaxGeometryInputComponents = */ 64,
    /* .MaxGeometryOutputComponents = */ 128,
    /* .MaxFragmentInputComponents = */ 128,
    /* .MaxImageUnits = */ 8,
    /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
    /* .MaxCombinedShaderOutputResources = */ 8,
    /* .MaxImageSamples = */ 0,
    /* .MaxVertexImageUniforms = */ 0,
    /* .MaxTessControlImageUniforms = */ 0,
    /* .MaxTessEvaluationImageUniforms = */ 0,
    /* .MaxGeometryImageUniforms = */ 0,
    /* .MaxFragmentImageUniforms = */ 8,
    /* .MaxCombinedImageUniforms = */ 8,
    /* .MaxGeometryTextureImageUnits = */ 16,
    /* .MaxGeometryOutputVertices = */ 256,
    /* .MaxGeometryTotalOutputComponents = */ 1024,
    /* .MaxGeometryUniformComponents = */ 1024,
    /* .MaxGeometryVaryingComponents = */ 64,
    /* .MaxTessControlInputComponents = */ 128,
    /* .MaxTessControlOutputComponents = */ 128,
    /* .MaxTessControlTextureImageUnits = */ 16,
    /* .MaxTessControlUniformComponents = */ 1024,
    /* .MaxTessControlTotalOutputComponents = */ 4096,
    /* .MaxTessEvaluationInputComponents = */ 128,
    /* .MaxTessEvaluationOutputComponents = */ 128,
    /* .MaxTessEvaluationTextureImageUnits = */ 16,
    /* .MaxTessEvaluationUniformComponents = */ 1024,
    /* .MaxTessPatchComponents = */ 120,
    /* .MaxPatchVertices = */ 32,
    /* .MaxTessGenLevel = */ 64,
    /* .MaxViewports = */ 16,
    /* .MaxVertexAtomicCounters = */ 0,
    /* .MaxTessControlAtomicCounters = */ 0,
    /* .MaxTessEvaluationAtomicCounters = */ 0,
    /* .MaxGeometryAtomicCounters = */ 0,
    /* .MaxFragmentAtomicCounters = */ 8,
    /* .MaxCombinedAtomicCounters = */ 8,
    /* .MaxAtomicCounterBindings = */ 1,
    /* .MaxVertexAtomicCounterBuffers = */ 0,
    /* .MaxTessControlAtomicCounterBuffers = */ 0,
    /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
    /* .MaxGeometryAtomicCounterBuffers = */ 0,
    /* .MaxFragmentAtomicCounterBuffers = */ 1,
    /* .MaxCombinedAtomicCounterBuffers = */ 1,
    /* .MaxAtomicCounterBufferSize = */ 16384,
    /* .MaxTransformFeedbackBuffers = */ 4,
    /* .MaxTransformFeedbackInterleavedComponents = */ 64,
    /* .MaxCullDistances = */ 8,
    /* .MaxCombinedClipAndCullDistances = */ 8,
    /* .MaxSamples = */ 4,
    /* .maxMeshOutputVerticesNV = */ 256,
    /* .maxMeshOutputPrimitivesNV = */ 512,
    /* .maxMeshWorkGroupSizeX_NV = */ 32,
    /* .maxMeshWorkGroupSizeY_NV = */ 1,
    /* .maxMeshWorkGroupSizeZ_NV = */ 1,
    /* .maxTaskWorkGroupSizeX_NV = */ 32,
    /* .maxTaskWorkGroupSizeY_NV = */ 1,
    /* .maxTaskWorkGroupSizeZ_NV = */ 1,
    /* .maxMeshViewCountNV = */ 4,
    /* .maxMeshOutputVerticesEXT = */ 256,
    /* .maxMeshOutputPrimitivesEXT = */ 256,
    /* .maxMeshWorkGroupSizeX_EXT = */ 128,
    /* .maxMeshWorkGroupSizeY_EXT = */ 128,
    /* .maxMeshWorkGroupSizeZ_EXT = */ 128,
    /* .maxTaskWorkGroupSizeX_EXT = */ 128,
    /* .maxTaskWorkGroupSizeY_EXT = */ 128,
    /* .maxTaskWorkGroupSizeZ_EXT = */ 128,
    /* .maxMeshViewCountEXT = */ 4,
    /* .maxDualSourceDrawBuffersEXT = */ 1,

    /* .limits = */ {
        /* .nonInductiveForLoops = */ 1,
        /* .whileLoops = */ 1,
        /* .doWhileLoops = */ 1,
        /* .generalUniformIndexing = */ 1,
        /* .generalAttributeMatrixVectorIndexing = */ 1,
        /* .generalVaryingIndexing = */ 1,
        /* .generalSamplerIndexing = */ 1,
        /* .generalVariableIndexing = */ 1,
        /* .generalConstantMatrixVectorIndexing = */ 1,
    }};
		return &DefaultTBuiltInResource;
	}

	// Modified get_bytecode method that can both load precompiled SPIR-V and compile GLSL to SPIR-V
	const std::vector<uint32_t> Shader::get_bytecode(std::string filename)
	{
		// Check if file is a precompiled SPIR-V (.spv) or a GLSL source file
		const std::string extension = filename.substr(filename.find_last_of('.') + 1);
		const bool isPrecompiledSpirv = (extension == "spv");

		if (isPrecompiledSpirv) {
			// Handle precompiled SPIR-V file
			std::ifstream file(filename, std::ios::ate | std::ios::binary);

			if (!file.is_open()) {
				throw std::runtime_error("failed to open file: " + filename);
			}

			const size_t file_size = (size_t)file.tellg();
			std::vector<uint32_t> bytecode(file_size / sizeof(uint32_t));

			file.seekg(0);
			file.read((char*)bytecode.data(), bytecode.size() * sizeof(uint32_t));
			file.close();
			return bytecode;
		} else {
			std::cerr << "Compiling GLSL shader: " << filename << std::endl;
			// Handle GLSL source file - compile to SPIR-V using glslang
			// Initialize glslang
			initializeGlslang();

			// Read shader source
			std::string shaderSource = readShaderFile(filename);
			
			// Create shader
			EShLanguage stage = findLanguageFromExtension(filename);
			glslang::TShader shader(stage);
			const char* shaderStrings[1] = { shaderSource.c_str() };
			shader.setStrings(shaderStrings, 1);

			// Set up compiler options
			int clientInputSemanticsVersion = 460; // Use GLSL 4.60 by default
			glslang::EShTargetClientVersion vulkanClientVersion = glslang::EShTargetVulkan_1_2;
			glslang::EShTargetLanguageVersion targetVersion = glslang::EShTargetSpv_1_5;
		
			shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, clientInputSemanticsVersion);
			shader.setEnvClient(glslang::EShClientVulkan, vulkanClientVersion);
			shader.setEnvTarget(glslang::EShTargetSpv, targetVersion);

			const TBuiltInResource* resources = getDefaultResources();
			EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

			DirStackFileIncluder includer;
			includer.pushExternalLocalDirectory("shaders");
			includer.pushExternalLocalDirectory("shaders/glsl");
			includer.pushExternalLocalDirectory("shaders/glsl/MeshShading");
			
			// Add the directory of the current shader file
			std::string shaderDir = filename.substr(0, filename.find_last_of("/\\"));
			if (!shaderDir.empty()) {
				includer.pushExternalLocalDirectory(shaderDir);
			}

			// Parse shader
			if (!shader.parse(resources, 100, false, messages, includer)) {
				throw std::runtime_error("Failed to parse GLSL shader: " + filename + 
					"\n" + shader.getInfoLog() + "\n" + shader.getInfoDebugLog());
			}

			// Link program
			glslang::TProgram program;
			program.addShader(&shader);
			
			if (!program.link(messages)) {
				throw std::runtime_error("Failed to link GLSL program: " + filename + 
					"\n" + program.getInfoLog() + "\n" + program.getInfoDebugLog());
			}

			// Generate SPIR-V
			std::vector<uint32_t> spirv;
			glslang::GlslangToSpv(*program.getIntermediate(stage), spirv);
			std::cerr << "SPIR-V generated for shader: " << filename << std::endl;
			return spirv;
		}
	}

	Shader::Shader(vk::Device logical_device, std::string shader_file)
	{
		const auto bytecode = get_bytecode(shader_file);
		init(logical_device, bytecode);
	}

	Shader::Shader(vk::Device logical_device, const std::vector<uint32_t>& bytecode)
	{
		init(logical_device, bytecode);
	}

	lz::ShaderModule* Shader::get_module()
	{
		return shader_module_.get();
	}

	vk::ShaderStageFlagBits Shader::get_stage_bits() const
	{
		return stage_flag_bits_;
	}



	size_t Shader::get_sets_count()
	{
		return descriptor_set_layout_keys_.size();
	}

	const DescriptorSetLayoutKey* Shader::get_set_info(size_t set_index)
	{
		return &descriptor_set_layout_keys_[set_index];
	}

	glm::uvec3 Shader::get_local_size()
	{
		return local_size_;
	}

	void Shader::init(vk::Device logical_device, const std::vector<uint32_t>& bytecode)
	{
		shader_module_.reset(new ShaderModule(logical_device, bytecode));
		local_size_ = glm::uvec3(0);
		spirv_cross::Compiler compiler(bytecode.data(), bytecode.size());

		auto entry_points = compiler.get_entry_points_and_stages();
		assert(entry_points.size() == 1);
		switch (entry_points[0].execution_model)
		{
		case spv::ExecutionModel::ExecutionModelVertex:
			{
				stage_flag_bits_ = vk::ShaderStageFlagBits::eVertex;
			}
			break;
		case spv::ExecutionModel::ExecutionModelFragment:
			{
				stage_flag_bits_ = vk::ShaderStageFlagBits::eFragment;
			}
			break;
		case spv::ExecutionModel::ExecutionModelGLCompute:
			{
				stage_flag_bits_ = vk::ShaderStageFlagBits::eCompute;
				local_size_.x = compiler.get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, 0);
				local_size_.y = compiler.get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, 1);
				local_size_.z = compiler.get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, 2);
			}
			break;

		case spv::ExecutionModel::ExecutionModelMeshEXT:
			{
				stage_flag_bits_ = vk::ShaderStageFlagBits::eMeshEXT;
				local_size_.x = compiler.get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, 0);
				local_size_.y = compiler.get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, 1);
				local_size_.z = compiler.get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, 2);
			}
			break;
		case spv::ExecutionModel::ExecutionModelTaskEXT:
			{
				stage_flag_bits_ = vk::ShaderStageFlagBits::eTaskEXT;
				local_size_.x = compiler.get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, 0);
				local_size_.y = compiler.get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, 1);
				local_size_.z = compiler.get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, 2);
			}
			break;

		default:
			{
				throw std::runtime_error("Unsupported shader stage");
				break;
			}
		}

		spirv_cross::ShaderResources resources = compiler.get_shader_resources();


		struct SetResources
		{
			std::vector<spirv_cross::Resource> uniform_buffers;
			std::vector<spirv_cross::Resource> image_samplers;
			std::vector<spirv_cross::Resource> storage_buffers;
			std::vector<spirv_cross::Resource> storage_images;
		};
		std::vector<SetResources> set_resources;
		for (const auto& buffer : resources.uniform_buffers)
		{
			uint32_t set_shader_id = compiler.get_decoration(buffer.id, spv::DecorationDescriptorSet);
			if (set_shader_id >= set_resources.size())
				set_resources.resize(set_shader_id + 1);
			set_resources[set_shader_id].uniform_buffers.push_back(buffer);
		}

		for (const auto& image_sampler : resources.sampled_images)
		{
			uint32_t set_shader_id = compiler.get_decoration(image_sampler.id, spv::DecorationDescriptorSet);
			if (set_shader_id >= set_resources.size())
				set_resources.resize(set_shader_id + 1);
			set_resources[set_shader_id].image_samplers.push_back(image_sampler);
		}

		for (const auto& buffer : resources.storage_buffers)
		{
			uint32_t set_shader_id = compiler.get_decoration(buffer.id, spv::DecorationDescriptorSet);
			if (set_shader_id >= set_resources.size())
				set_resources.resize(set_shader_id + 1);
			set_resources[set_shader_id].storage_buffers.push_back(buffer);
		}

		for (const auto& image : resources.storage_images)
		{
			uint32_t set_shader_id = compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
			if (set_shader_id >= set_resources.size())
				set_resources.resize(set_shader_id + 1);
			set_resources[set_shader_id].storage_images.push_back(image);
		}

		this->descriptor_set_layout_keys_.resize(set_resources.size());
		for (size_t set_index = 0; set_index < set_resources.size(); set_index++)
		{
			auto& descriptor_set_layout_key = descriptor_set_layout_keys_[set_index];
			descriptor_set_layout_key = DescriptorSetLayoutKey();
			descriptor_set_layout_key.set_shader_id_ = uint32_t(set_index);

			for (auto& buffer : set_resources[set_index].uniform_buffers)
			{
				uint32_t shader_binding_index = compiler.get_decoration(buffer.id, spv::DecorationBinding);

				auto buffer_type = compiler.get_type(buffer.type_id);

				if (buffer_type.basetype == spirv_cross::SPIRType::BaseType::Struct)
				{
					auto uniform_buffer_id = DescriptorSetLayoutKey::UniformBufferId(
						descriptor_set_layout_key.uniform_buffer_datum_.size());
					descriptor_set_layout_key.uniform_buffer_datum_.emplace_back(
						DescriptorSetLayoutKey::UniformBufferData());
					auto& buffer_data = descriptor_set_layout_key.uniform_buffer_datum_.back();

					buffer_data.shader_binding_index = shader_binding_index;
					buffer_data.name = buffer.name;
					buffer_data.stage_flags = stage_flag_bits_;

					uint32_t declared_size = uint32_t(compiler.get_declared_struct_size(buffer_type));

					uint32_t curr_offset = 0;
					for (uint32_t member_index = 0; member_index < buffer_type.member_types.size(); member_index++)
					{
						uint32_t member_size = uint32_t(
							compiler.get_declared_struct_member_size(buffer_type, member_index));
						//auto memberType = compiler.get_type(bufferType.member_types[memberIndex]);
						auto member_name = compiler.get_member_name(buffer.base_type_id, member_index);

						//memberData.size = compiler.get_declared_struct_size(memeberType);
						DescriptorSetLayoutKey::UniformId uniform_id = DescriptorSetLayoutKey::UniformId(
							descriptor_set_layout_key.uniform_datum_.size());
						descriptor_set_layout_key.uniform_datum_.push_back(DescriptorSetLayoutKey::UniformData());
						DescriptorSetLayoutKey::UniformData& uniformData = descriptor_set_layout_key.uniform_datum_.
							back();

						uniformData.uniform_buffer_id = uniform_buffer_id;
						uniformData.offset_in_binding = uint32_t(curr_offset);
						uniformData.size = uint32_t(member_size);
						uniformData.name = member_name;

						//memberData.
						buffer_data.uniform_ids.push_back(uniform_id);

						curr_offset += member_size;
					}
					assert(curr_offset == declared_size);
					//alignment is wrong. avoid using smaller types before larger ones. completely avoid vec2/vec3
					buffer_data.size = declared_size;
					buffer_data.offset_in_set = descriptor_set_layout_key.size_;

					descriptor_set_layout_key.size_ += buffer_data.size;
				}
			}

			for (auto image_sampler : set_resources[set_index].image_samplers)
			{
				auto image_sampler_id = DescriptorSetLayoutKey::ImageSamplerId(
					descriptor_set_layout_key.image_sampler_datum_.size());

				uint32_t shader_binding_index = compiler.get_decoration(image_sampler.id, spv::DecorationBinding);
				descriptor_set_layout_key.image_sampler_datum_.push_back(DescriptorSetLayoutKey::ImageSamplerData());
				auto& image_sampler_data = descriptor_set_layout_key.image_sampler_datum_.back();
				image_sampler_data.shader_binding_index = shader_binding_index;
				image_sampler_data.stage_flags = stage_flag_bits_;
				image_sampler_data.name = image_sampler.name;
			}

			for (auto buffer : set_resources[set_index].storage_buffers)
			{
				uint32_t shader_binding_index = compiler.get_decoration(buffer.id, spv::DecorationBinding);

				auto buffer_type = compiler.get_type(buffer.type_id);

				if (buffer_type.basetype == spirv_cross::SPIRType::BaseType::Struct)
				{
					auto storage_buffer_id = DescriptorSetLayoutKey::StorageBufferId(
						descriptor_set_layout_key.storage_buffer_datum_.size());
					descriptor_set_layout_key.storage_buffer_datum_.emplace_back(
						DescriptorSetLayoutKey::StorageBufferData());
					auto& buffer_data = descriptor_set_layout_key.storage_buffer_datum_.back();

					buffer_data.shader_binding_index = shader_binding_index;
					buffer_data.stage_flags = stage_flag_bits_;
					buffer_data.name = buffer.name;
					buffer_data.pod_part_size = 0;
					buffer_data.array_member_size = 0;
					buffer_data.offset_in_set = 0; //should not be used
					size_t declared_size = compiler.get_declared_struct_size(buffer_type);

					uint32_t curr_offset = 0;
					buffer_data.pod_part_size = uint32_t(compiler.get_declared_struct_size(buffer_type));

					//taken from get_declared_struct_size_runtime_array implementation
					auto& last_type = compiler.get_type(buffer_type.member_types.back());
					if (!last_type.array.empty() && last_type.array_size_literal[0] && last_type.array[0] == 0)
						// Runtime array
						buffer_data.array_member_size = compiler.type_struct_member_array_stride(
							buffer_type, uint32_t(buffer_type.member_types.size() - 1));
					else
						buffer_data.array_member_size = 0;
				}
			}

			for (auto image : set_resources[set_index].storage_images)
			{
				auto image_sampler_id = DescriptorSetLayoutKey::ImageSamplerId(
					descriptor_set_layout_key.image_sampler_datum_.size());

				uint32_t shader_binding_index = compiler.get_decoration(image.id, spv::DecorationBinding);
				descriptor_set_layout_key.storage_image_datum_.push_back(DescriptorSetLayoutKey::StorageImageData());
				auto& storage_image_data = descriptor_set_layout_key.storage_image_datum_.back();
				storage_image_data.shader_binding_index = shader_binding_index;
				storage_image_data.stage_flags = stage_flag_bits_;
				storage_image_data.name = image.name;
				//type?
			}
			descriptor_set_layout_key.rebuild_index();
		}
	}

	ShaderProgram::ShaderProgram(std::initializer_list<Shader*> shaders)
	{
		size_t max_sets_count = 0;
		for (auto& shader:shaders)
		{
			this->shaders.push_back(shader);
			max_sets_count = std::max(max_sets_count, shader->get_sets_count());
		}
		combined_descriptor_set_layout_keys.resize(max_sets_count);
		for (size_t set_index = 0; set_index  < combined_descriptor_set_layout_keys.size(); ++set_index)
		{
			vk::DescriptorSetLayout set_layout_handle = nullptr;
			std::vector<lz::DescriptorSetLayoutKey> set_layout_stage_keys;
			for (auto& shader : shaders)
			{
				if (set_index < shader->get_sets_count())
				{
					auto set_info = shader->get_set_info(set_index);
					if (!set_info->is_empty())
					{
						set_layout_stage_keys.push_back(*set_info);
					}
				}
			}
			this->combined_descriptor_set_layout_keys[set_index] = DescriptorSetLayoutKey::merge(
				set_layout_stage_keys.data(), set_layout_stage_keys.size());
		}

	}

	size_t ShaderProgram::get_sets_count()
	{
		return combined_descriptor_set_layout_keys.size();
	}

	const DescriptorSetLayoutKey* ShaderProgram::get_set_info(size_t set_index)
	{
		return &combined_descriptor_set_layout_keys[set_index];
	}
}
