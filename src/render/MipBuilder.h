#pragma once

#include "backend/RenderGraph.h"
#include "backend/Sampler.h"
#include "backend/ShaderProgram.h"

namespace lz
{
class ShaderMemoryPool;
class Core;
}        // namespace lz

namespace lz::render
{

/**
 * UnmippedImageProxy - Structure to manage unmipped images in the render pipeline
 * Used for handling images that need mipmap generation
 */
struct UnmippedImageProxy
{
	// Constructor for creating an unmipped image proxy with specified format, size and usage flags
	UnmippedImageProxy(lz::RenderGraph *render_graph, vk::Format format, glm::uvec2 base_size, vk::ImageUsageFlags usage_flags);

	lz::RenderGraph::ImageProxyUnique     image_proxy;             // The image resource in the render graph
	lz::RenderGraph::ImageViewProxyUnique image_view_proxy;        // The image view resource in the render graph
	glm::uvec2                            base_size;               // Base dimensions of the image
};

struct MippedImageProxy
{
	MippedImageProxy(lz::RenderGraph *render_graph, vk::Format format, glm::uvec2 base_size, vk::ImageUsageFlags usage_flags);

	lz::RenderGraph::ImageProxyUnique                  image_proxy;
	lz::RenderGraph::ImageViewProxyUnique              image_view_proxy;
	std::vector<lz::RenderGraph::ImageViewProxyUnique> mip_image_view_proxies;
	glm::uvec2                                         base_size;
};

class MipBuilder
{
  public:
	enum struct FilterTypes
	{
		eAvg,
		eDepth
	};

	MipBuilder(lz::Core *core);

	void build_mips(lz::RenderGraph *render_graph, lz::ShaderMemoryPool *memory_pool, const MippedImageProxy &mipped_proxy, FilterTypes filter_type = FilterTypes::eAvg);

	void reload_shader();

  private:
	constexpr static uint32_t k_shader_data_set_index    = 0;

	struct MipLevelBuilder
	{
#pragma pack(push, 1)
		struct ShaderDataBuffer
		{
			float filter_type;
		};
#pragma pack(pop)
		std::unique_ptr<lz::Shader>        vertex_shader;
		std::unique_ptr<lz::Shader>        fragment_shader;
		std::unique_ptr<lz::ShaderProgram> shader_program;
	} mip_level_builder_;

	std::unique_ptr<lz::Sampler> image_space_sampler_;
	lz::Core                    *core_;
};
}        // namespace lz::render