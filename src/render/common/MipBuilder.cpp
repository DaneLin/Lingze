#include "MipBuilder.h"

namespace lz::render
{
	UnmippedImageProxy::UnmippedImageProxy(lz::RenderGraph* render_graph, vk::Format format, glm::uvec2 base_size,
		vk::ImageUsageFlags usage_flags)
			:base_size(base_size)
	{
		image_proxy = render_graph->add_image(format, 1, 1, base_size, usage_flags);
		image_view_proxy = render_graph->add_image_view(image_proxy->id(), 0, 1, 0, 1);
	}
}
