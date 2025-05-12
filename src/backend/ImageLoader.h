#pragma once
#include <gli/gli.hpp>

#include "Config.h"
#include "Core.h"
#include "PresentQueue.h"

namespace lz
{
struct ImageTexelData
{
	size_t     layers_count = 0;
	vk::Format format       = vk::Format::eUndefined;
	size_t     texel_size   = 0;
	glm::uvec3 base_size    = {0, 0, 0};

	struct Mip
	{
		glm::uvec3 size;

		struct Layer
		{
			size_t offset;
		};

		std::vector<Layer> layers;
	};

	std::vector<Mip>     mips;
	std::vector<uint8_t> texels;
};

static ImageTexelData create_test_cube_texel_data()
{
	glm::uvec3     size = glm::uvec3(64, 64, 1);
	ImageTexelData texel_data;
	texel_data.layers_count = 6;
	texel_data.format       = vk::Format::eR8G8B8A8Snorm;
	texel_data.texel_size   = 4;
	texel_data.mips.resize(1);
	texel_data.mips[0].layers.resize(texel_data.layers_count);
	texel_data.base_size = size;

	texel_data.texels.resize(size.x * size.y * texel_data.layers_count * texel_data.texel_size);

	size_t curr_offset = 0;
	for (auto &mip : texel_data.mips)
	{
		mip.size = size;
		for (auto &layer : mip.layers)
		{
			layer.offset = curr_offset;
			for (size_t z = 0; z < size.z; z++)
			{
				for (size_t y = 0; y < size.y; y++)
				{
					for (size_t x = 0; x < size.x; x++)
					{
						uint8_t *texel_ptr = texel_data.texels.data() + curr_offset + (x + y * mip.size.x + z * mip.size.x * mip.size.y) * texel_data.texel_size;
						texel_ptr[0]       = uint8_t(x);
						texel_ptr[1]       = uint8_t(y);
						texel_ptr[2]       = 0;
						texel_ptr[3]       = 0;
					}
				}
			}
			curr_offset += mip.size.x * mip.size.y * texel_data.texel_size;
		}
	}
	assert(curr_offset == texel_data.texels.size());
	return texel_data;
}

static ImageTexelData load_texel_data_from_gli(const gli::texture &texture)
{
	const glm::uvec3 size = {texture.extent().x, texture.extent().y, texture.extent().z};
	ImageTexelData   texel_data;
	const size_t     mips_count = texture.max_level() + 1;
	texel_data.layers_count     = texture.max_face() + 1;
	switch (texture.format())
	{
		case gli::format::FORMAT_RGBA8_UINT_PACK8:
		case gli::format::FORMAT_RGBA8_SRGB_PACK8:
		{
			texel_data.format     = vk::Format::eR8G8B8A8Srgb;
			texel_data.texel_size = 4 * sizeof(uint8_t);
		}
		break;
		case gli::format::FORMAT_RGB8_UINT_PACK8:
		{
			texel_data.format     = vk::Format::eR8G8B8Srgb;
			texel_data.texel_size = 3 * sizeof(uint8_t);
		}
		break;
		case gli::format::FORMAT_RGBA8_UNORM_PACK8:
		{
			texel_data.format     = vk::Format::eR8G8B8A8Unorm;
			texel_data.texel_size = 4 * sizeof(uint8_t);
		}
		break;
		case gli::format::FORMAT_RGBA32_SFLOAT_PACK32:
		{
			texel_data.format     = vk::Format::eR32G32B32A32Sfloat;
			texel_data.texel_size = 4 * sizeof(float);
		}
		break;
		case gli::format::FORMAT_RGBA16_SFLOAT_PACK16:
		{
			texel_data.format     = vk::Format::eR16G16B16A16Sfloat;
			texel_data.texel_size = 2 * sizeof(float);
		}
		break;
		default:
		{
			assert(0);
		}
		break;
	}
	texel_data.mips.resize(mips_count);
	texel_data.base_size = size;
	texel_data.texels.resize(/*size.x * size.y * texelData.layersCount * texelData.texelSize*/ texture.size());

	size_t curr_offset = 0;
	for (size_t mip_level = 0; mip_level < mips_count; mip_level++)
	{
		auto &mip = texel_data.mips[mip_level];
		mip.layers.resize(texel_data.layers_count);

		mip.size = {texture.extent(mip_level).x, texture.extent(mip_level).y, texture.extent(mip_level).z};
		for (size_t faceIndex = 0; faceIndex < texel_data.layers_count; faceIndex++)
		{
			auto &layer  = mip.layers[faceIndex];
			layer.offset = curr_offset;

			uint8_t       *dst_texels_ptr = texel_data.texels.data() + curr_offset;
			const uint8_t *src_texels_ptr = (uint8_t *) texture.data(0, faceIndex, mip_level);

			memcpy(dst_texels_ptr, src_texels_ptr, mip.size.x * mip.size.y * mip.size.z * texel_data.texel_size);
			/*for (size_t z = 0; z < size.z; z++)
			{
			  for (size_t y = 0; y < size.y; y++)
			  {
			    for (size_t x = 0; x < size.x; x++)
			    {
			      size_t localOffset = (x + y * mip.size.x + z * mip.size.x * mip.size.y) * texelData.texelSize;

			      struct RGB32f
			      {
			        float r;
			        float g;
			        float b;
			      };

			      RGB32f *srcTexelRgb32f = (RGB32f*)(srcTexelsPtr + localOffset);
			      RGB32f *dstTexelRgb32f = (RGB32f*)(dstTexelsPtr + localOffset);
			      *dstTexelRgb32f = *srcTexelRgb32f;
			    }
			  }
			}*/
			curr_offset += mip.size.x * mip.size.y * texel_data.texel_size;
		}
	}
	assert(texel_data.texels.size() == curr_offset);
	return texel_data;
}

static ImageTexelData load_ktx_from_file(const std::string &filename)
{
	gli::texture texture = gli::load(filename);
	if (!texture.empty())
		return load_texel_data_from_gli(texture);
	else
		return ImageTexelData();
}

static gli::texture load_texel_data_to_gli(ImageTexelData texel_data)
{
	gli::texture::format_type format;
	switch (texel_data.format)
	{
		case vk::Format::eR8G8B8A8Unorm:
		{
			format = gli::format::FORMAT_RGBA8_UNORM_PACK8;
		}
		break;
		case vk::Format::eR8G8B8A8Srgb:
		{
			format = gli::format::FORMAT_RGBA8_SRGB_PACK8;
		}
		break;
		case vk::Format::eR32G32B32A32Sfloat:
		{
			format = gli::format::FORMAT_RGBA32_SFLOAT_PACK32;
		}
		break;
		case vk::Format::eR16G16B16A16Sfloat:
		{
			format = gli::format::FORMAT_RGBA16_SFLOAT_PACK16;
		}
		break;
		default:
		{
			assert(0);
		}
		break;
	}
	// gli::target::target_type
	gli::texture texture(gli::target::TARGET_2D, format,
	                     gli::extent3d(texel_data.base_size.x, texel_data.base_size.y, texel_data.base_size.z),
	                     texel_data.layers_count, 1, texel_data.mips.size());

	for (size_t mip_level = 0; mip_level < texel_data.mips.size(); mip_level++)
	{
		auto &mip = texel_data.mips[mip_level];
		mip.size  = {texture.extent(mip_level).x, texture.extent(mip_level).y, texture.extent(mip_level).z};
		for (size_t layer_index = 0; layer_index < texel_data.layers_count; layer_index++)
		{
			auto        &layer       = mip.layers[layer_index];
			const size_t curr_offset = layer.offset;

			const uint8_t *src_texels_ptr = texel_data.texels.data() + curr_offset;
			uint8_t       *dst_texels_ptr = (uint8_t *) texture.data(layer_index, 0, mip_level);

			memcpy(dst_texels_ptr, src_texels_ptr, mip.size.x * mip.size.y * mip.size.z * texel_data.texel_size);
		}
	}
	return texture;
}

static void save_ktx_to_file(const ImageTexelData &data, const std::string &filename)
{
	const auto gli_data = lz::load_texel_data_to_gli(data);
	gli::save(gli_data, filename);
}

static size_t get_format_size(const vk::Format format)
{
	switch (format)
	{
		case vk::Format::eR8G8B8A8Unorm:
			return 4;
			break;
		case vk::Format::eR32G32B32A32Sfloat:
			return 4 * sizeof(float);
			break;
		case vk::Format::eR16G16B16A16Sfloat:
			return 2 * sizeof(float);
			break;
	}
	assert(0);
	return -1;
}

static lz::ImageTexelData create_simple_image_texel_data(const glm::uint8 *pixels, const int width,
                                                         const int        height,
                                                         const vk::Format format = vk::Format::eR8G8B8A8Unorm)
{
	lz::ImageTexelData texel_data;
	texel_data.base_size    = glm::uvec3(width, height, 1);
	texel_data.format       = format;
	texel_data.layers_count = 1;
	texel_data.mips.resize(1);
	texel_data.mips[0].size = texel_data.base_size;
	texel_data.mips[0].layers.resize(texel_data.layers_count);
	texel_data.mips[0].layers[0].offset = 0;
	texel_data.texel_size               = get_format_size(format);
	texel_data.texels.resize(width * height * texel_data.texel_size);
	const size_t total_size = width * height * texel_data.texel_size;
	memcpy(texel_data.texels.data(), pixels, total_size);
	return texel_data;
}

static void add_transition_barrier(const lz::ImageData *image_data, const lz::ImageUsageTypes src_usage_type,
                                   const lz::ImageUsageTypes dst_usage_type, const vk::CommandBuffer command_buffer)
{
	const auto src_image_access_pattern = get_src_image_access_pattern(src_usage_type);
	const auto dst_image_access_pattern = get_dst_image_access_pattern(dst_usage_type);

	const auto range = vk::ImageSubresourceRange()
	                       .setAspectMask(image_data->get_aspect_flags())
	                       .setBaseArrayLayer(0)
	                       .setLayerCount(image_data->get_array_layers_count())
	                       .setBaseMipLevel(0)
	                       .setLevelCount(image_data->get_mips_count());

	auto image_barrier = vk::ImageMemoryBarrier()
	                         .setSrcAccessMask(src_image_access_pattern.access_mask)
	                         .setOldLayout(src_image_access_pattern.layout)
	                         .setDstAccessMask(dst_image_access_pattern.access_mask)
	                         .setNewLayout(dst_image_access_pattern.layout)
	                         .setSubresourceRange(range)
	                         .setImage(image_data->get_handle());

	image_barrier
	    .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
	    .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);

	command_buffer.pipelineBarrier(src_image_access_pattern.stage, dst_image_access_pattern.stage,
	                               vk::DependencyFlags(),
	                               {}, {}, {image_barrier});
}

static void load_texel_data(lz::Core *core, const ImageTexelData *texel_data, const lz::ImageData *dstImageData,
                            const lz::ImageUsageTypes dstUsageType = lz::ImageUsageTypes::eGraphicsShaderRead)
{
	const auto staging_buffer = std::make_unique<lz::Buffer>(core->get_physical_device(),
	                                                         core->get_logical_device(),
	                                                         texel_data->texels.size(),
	                                                         vk::BufferUsageFlagBits::eTransferSrc,
	                                                         vk::MemoryPropertyFlagBits::eHostVisible |
	                                                             vk::MemoryPropertyFlagBits::eHostCoherent);

	void *buffer_data = staging_buffer->map();
	memcpy(buffer_data, texel_data->texels.data(), texel_data->texels.size());
	staging_buffer->unmap();

	std::vector<vk::BufferImageCopy> copy_regions;
	for (uint32_t mip_level = 0; mip_level < uint32_t(texel_data->mips.size()); mip_level++)
	{
		const auto &mip = texel_data->mips[mip_level];

		for (uint32_t array_layer = 0; array_layer < uint32_t(mip.layers.size()); array_layer++)
		{
			const auto &layer             = mip.layers[array_layer];
			auto        image_subresource = vk::ImageSubresourceLayers()
			                             .setAspectMask(vk::ImageAspectFlagBits::eColor)
			                             .setMipLevel(mip_level)
			                             .setBaseArrayLayer(array_layer)
			                             .setLayerCount(1);

			auto copy_region = vk::BufferImageCopy()
			                       .setBufferOffset(layer.offset)
			                       .setBufferRowLength(0)
			                       .setBufferImageHeight(0)
			                       .setImageSubresource(image_subresource)
			                       .setImageOffset(vk::Offset3D(0, 0, 0))
			                       .setImageExtent(vk::Extent3D(mip.size.x, mip.size.y, mip.size.z));

			copy_regions.push_back(copy_region);
		}
	}

	lz::ExecuteOnceQueue transfer_queue(core);
	{
		const auto transfer_command_buffer = transfer_queue.begin_command_buffer();
		add_transition_barrier(dstImageData, lz::ImageUsageTypes::eNone, lz::ImageUsageTypes::eTransferDst, transfer_command_buffer);
		transfer_command_buffer.copyBufferToImage(staging_buffer->get_handle(), dstImageData->get_handle(), vk::ImageLayout::eTransferDstOptimal, copy_regions);
		add_transition_barrier(dstImageData, lz::ImageUsageTypes::eTransferDst, dstUsageType, transfer_command_buffer);
	}
	transfer_queue.end_command_buffer();
}
}        // namespace lz
