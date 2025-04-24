#pragma once

#include "backend/RenderGraph.h"

namespace lz::render
{
	struct UnmippedProxy
	{
		UnmippedProxy(lz::RenderGraph* render_graph, vk::Format format, glm::uvec2 base_size, vk::ImageUsageFlags usage_flags);

		lz::RenderGraph::ImageProxyUnique image_proxy;
		lz::RenderGraph::ImageViewProxyUnique image_view_proxy;
		glm::uvec2 base_size;
	};
}