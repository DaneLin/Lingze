#pragma once

#include "backend/RenderGraph.h"

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

	lz::RenderGraph::ImageProxyUnique     image_proxy;      // The image resource in the render graph
	lz::RenderGraph::ImageViewProxyUnique image_view_proxy; // The image view resource in the render graph
	glm::uvec2                            base_size;        // Base dimensions of the image
};
}        // namespace lz::render