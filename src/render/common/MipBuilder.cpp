#include "MipBuilder.h"

namespace lz::render
{
// Implementation of UnmippedImageProxy constructor
// Creates image and image view resources in the render graph with the specified parameters
UnmippedImageProxy::UnmippedImageProxy(lz::RenderGraph *render_graph, vk::Format format, glm::uvec2 base_size,
                                       vk::ImageUsageFlags usage_flags) :
    base_size(base_size)
{
	// Create a new image in the render graph with single mip level and array layer
	image_proxy      = render_graph->add_image(format, 1, 1, base_size, usage_flags);
	// Create an image view for the entire image (single mip level and array layer)
	image_view_proxy = render_graph->add_image_view(image_proxy->id(), 0, 1, 0, 1);
}
}        // namespace lz::render
