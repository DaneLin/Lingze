#include "MipBuilder.h"

#include "backend/Core.h"
#include "backend/MathUtils.h"

namespace lz::render
{
// Implementation of UnmippedImageProxy constructor
// Creates image and image view resources in the render graph with the specified parameters
UnmippedImageProxy::UnmippedImageProxy(lz::RenderGraph *render_graph, vk::Format format, glm::uvec2 base_size, vk::ImageUsageFlags usage_flags) :
    base_size(base_size)
{
	image_proxy      = render_graph->add_image(format, 1, 1, base_size, usage_flags);
	image_view_proxy = render_graph->add_image_view(image_proxy->id(), 0, 1, 0, 1);
}
MippedImageProxy::MippedImageProxy(lz::RenderGraph *render_graph, vk::Format format, glm::uvec2 base_size, vk::ImageUsageFlags usage_flags) :
    base_size(base_size)
{
	base_size           = glm::vec2(lz::math::previous_power_of_two(base_size.x), lz::math::previous_power_of_two(base_size.y));
	uint32_t mip_levels = lz::math::get_mip_levels(base_size.x, base_size.y);

	image_proxy      = render_graph->add_image(format, mip_levels, 1, base_size, usage_flags);
	image_view_proxy = render_graph->add_image_view(image_proxy->id(), 0, mip_levels, 0, 1);

	mip_image_view_proxies.reserve(mip_levels);
	for (uint32_t mip_index = 0; mip_index < mip_levels; ++mip_index)
	{
		mip_image_view_proxies.emplace_back(render_graph->add_image_view(image_proxy->id(), mip_index, 1, 0, 1));
	}
}

MipBuilder::MipBuilder(lz::Core *core) :
    core_(core)
{
	image_space_sampler_.reset(new lz::Sampler(core_->get_logical_device(), vk::SamplerAddressMode::eClampToEdge, vk::Filter::eNearest, vk::SamplerMipmapMode::eNearest));
	reload_shader();
}
void MipBuilder::build_mips(lz::RenderGraph *render_graph, lz::ShaderMemoryPool *memory_pool, const MippedImageProxy &mipped_proxy, FilterTypes filter_type)
{
	vk::Extent2D layer_size(lz::math::previous_power_of_two(mipped_proxy.base_size.x), lz::math::previous_power_of_two(mipped_proxy.base_size.y));

	for (size_t mip_index = 1; mip_index < mipped_proxy.mip_image_view_proxies.size(); ++mip_index)
	{
		layer_size.width  = std::max(1u, layer_size.width / 2);
		layer_size.height = std::max(1u, layer_size.height / 2);

		auto src_proxy_id = mipped_proxy.mip_image_view_proxies[mip_index - 1]->id();
		auto dst_proxy_id = mipped_proxy.mip_image_view_proxies[mip_index]->id();

		render_graph->add_pass(
		    lz::RenderGraph::RenderPassDesc()
		        .set_color_attachments({dst_proxy_id})
		        .set_input_images({src_proxy_id})
		        .set_render_area_extent(layer_size)
		        .set_profiler_info(lz::Colors::nephritis, "MipBuilderPass")
		        .set_record_func([this, memory_pool, src_proxy_id, mip_index, filter_type](lz::RenderGraph::RenderPassContext pass_context) {
			        auto pipeineInfo = this->core_->get_pipeline_cache()->bind_graphics_pipeline(
			            pass_context.get_command_buffer(),
			            pass_context.get_render_pass()->get_handle(),
			            lz::DepthSettings::disabled(),
			            {lz::BlendSettings::opaque()},
			            lz::VertexDeclaration(),
			            vk::PrimitiveTopology::eTriangleFan,
			            mip_level_builder_.shader_program.get());

			        {
				        const lz::DescriptorSetLayoutKey *shader_data_set_info     = mip_level_builder_.fragment_shader->get_set_info(k_shader_data_set_index);
				        auto                              dynamic_uniform_bindings = memory_pool->begin_set(shader_data_set_info);
				        {
					        auto shader_data_buffer         = memory_pool->get_uniform_buffer_data<MipLevelBuilder::ShaderDataBuffer>("MipLevelBuilderData");
					        shader_data_buffer->filter_type = (filter_type == FilterTypes::eAvg) ? 0.0f : 1.0f;
				        }
				        memory_pool->end_set();

				        std::vector<lz::ImageSamplerBinding> image_sampler_bindings;
				        auto                                 prev_mip_view = pass_context.get_image_view(src_proxy_id);
				        image_sampler_bindings.push_back(shader_data_set_info->make_image_sampler_binding("prev_level_sampler", prev_mip_view, image_space_sampler_.get()));
				        auto shader_data_set = this->core_->get_descriptor_set_cache()->get_descriptor_set(*shader_data_set_info, dynamic_uniform_bindings.uniform_buffer_bindings, {}, {}, image_sampler_bindings);
				        pass_context.get_command_buffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeineInfo.pipeline_layout, k_shader_data_set_index, {shader_data_set}, {dynamic_uniform_bindings.dynamic_offset});
				        pass_context.get_command_buffer().draw(4, 1, 0, 0);
			        }
		        }));
	}
}
void MipBuilder::reload_shader()
{
	mip_level_builder_.vertex_shader.reset(new lz::Shader(core_->get_logical_device(), SHADER_GLSL_DIR "Common/screen_quad.vert"));
	mip_level_builder_.fragment_shader.reset(new lz::Shader(core_->get_logical_device(), SHADER_GLSL_DIR "Common/mip_builder.frag"));
	mip_level_builder_.shader_program.reset(new lz::ShaderProgram({mip_level_builder_.vertex_shader.get(), mip_level_builder_.fragment_shader.get()}));
}
}        // namespace lz::render
