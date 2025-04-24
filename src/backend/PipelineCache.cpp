#include "PipelineCache.h"

#include "DescriptorSetCache.h"
#include "Pipeline.h"
#include "ShaderModule.h"
#include "ShaderProgram.h"

namespace lz
{
	PipelineCache::PipelineCache(vk::Device logical_device, DescriptorSetCache* descriptor_set_cache) :
		logical_device_(logical_device),
		descriptor_set_cache_(descriptor_set_cache)
	{
	}

	PipelineCache::PipelineInfo PipelineCache::bind_graphics_pipeline(vk::CommandBuffer command_buffer,
	                                                                  vk::RenderPass render_pass,
	                                                                  lz::DepthSettings depth_settings,
	                                                                  const std::vector<lz::BlendSettings>&
	                                                                  attachment_blend_settings,
	                                                                  lz::VertexDeclaration vertex_declaration,
	                                                                  vk::PrimitiveTopology topology,
	                                                                  lz::ShaderProgram* shader_program)
	{
		GraphicsPipelineKey pipeline_key;
		pipeline_key.vertex_shader = shader_program->vertex_shader->get_module()->get_handle();
		pipeline_key.fragment_shader = shader_program->fragment_shader->get_module()->get_handle();
		pipeline_key.vertex_decl = vertex_declaration;
		pipeline_key.depth_settings = depth_settings;
		pipeline_key.attachment_blend_settings = attachment_blend_settings;
		pipeline_key.topology = topology;

		PipelineInfo pipeline_info;

		lz::Shader* target_shader = shader_program->vertex_shader;

		PipelineLayoutKey pipeline_layout_key;
		for (auto& set_layout_key : shader_program->combined_descriptor_set_layout_keys)
		{
			pipeline_layout_key.set_layouts.push_back(descriptor_set_cache_->get_descriptor_set_layout(set_layout_key));
		}

		pipeline_key.pipeline_layout = get_pipeline_layout(pipeline_layout_key);

		pipeline_key.render_pass = render_pass;

		lz::GraphicsPipeline* pipeline = get_graphics_pipeline(pipeline_key);

		command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->get_handle());

		pipeline_info.pipeline_layout = pipeline->get_layout();
		return pipeline_info;
	}

	PipelineCache::PipelineInfo PipelineCache::bind_compute_pipeline(vk::CommandBuffer command_buffer,
	                                                                 lz::Shader* compute_shader)
	{
		ComputePipelineKey pipeline_key;
		pipeline_key.compute_shader = compute_shader->get_module()->get_handle();

		PipelineInfo pipeline_info;
		lz::Shader* target_shader = compute_shader;

		PipelineLayoutKey pipeline_layout_key;
		pipeline_layout_key.set_layouts.resize(compute_shader->get_sets_count());
		for (size_t set_index = 0; set_index < pipeline_layout_key.set_layouts.size(); set_index++)
		{
			vk::DescriptorSetLayout set_layout_handle = nullptr;
			const auto compute_set_info = compute_shader->get_set_info(set_index);
			if (!compute_set_info->is_empty())
				set_layout_handle = descriptor_set_cache_->get_descriptor_set_layout(*compute_set_info);

			pipeline_layout_key.set_layouts[set_index] = set_layout_handle;
		}
		pipeline_key.pipeline_layout = get_pipeline_layout(pipeline_layout_key);

		lz::ComputePipeline* pipeline = get_compute_pipeline(pipeline_key);

		command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline->get_handle());

		pipeline_info.pipeline_layout = pipeline->get_layout();
		return pipeline_info;
	}

	void PipelineCache::clear()
	{
		this->compute_pipeline_cache_.clear();
		this->graphics_pipeline_cache_.clear();
		this->pipeline_layout_cache_.clear();
	}

	bool PipelineCache::PipelineLayoutKey::operator<(const PipelineLayoutKey& other) const
	{
		return
			std::tie(set_layouts) < std::tie(other.set_layouts);
	}

	vk::UniquePipelineLayout PipelineCache::create_pipeline_layout(
		const std::vector<vk::DescriptorSetLayout>& setLayouts)
	{
		const auto pipeline_layout_info = vk::PipelineLayoutCreateInfo()
		                                  .setPushConstantRangeCount(0)
		                                  .setPPushConstantRanges(nullptr)
		                                  .setSetLayoutCount(uint32_t(setLayouts.size()))
		                                  .setPSetLayouts(setLayouts.data());

		return logical_device_.createPipelineLayoutUnique(pipeline_layout_info);
	}

	vk::PipelineLayout PipelineCache::get_pipeline_layout(const PipelineLayoutKey& key)
	{
		auto& pipeline_layout = pipeline_layout_cache_[key];
		if (!pipeline_layout)
			pipeline_layout = create_pipeline_layout(key.set_layouts);
		return pipeline_layout.get();
	}

	PipelineCache::GraphicsPipelineKey::GraphicsPipelineKey()
	{
		vertex_shader = nullptr;
		fragment_shader = nullptr;
		render_pass = nullptr;
	}

	bool PipelineCache::GraphicsPipelineKey::operator<(const GraphicsPipelineKey& other) const
	{
		return
			std::tie(vertex_shader, fragment_shader, vertex_decl, pipeline_layout, render_pass, depth_settings,
			         attachment_blend_settings, topology) <
			std::tie(other.vertex_shader, other.fragment_shader, other.vertex_decl, other.pipeline_layout,
			         other.render_pass, other.depth_settings, other.attachment_blend_settings, topology);
	}

	lz::GraphicsPipeline* PipelineCache::get_graphics_pipeline(const GraphicsPipelineKey& key)
	{
		auto& pipeline = graphics_pipeline_cache_[key];
		if (!pipeline)
			pipeline = std::make_unique<lz::GraphicsPipeline>(
				logical_device_, key.vertex_shader, key.fragment_shader, key.vertex_decl, key.pipeline_layout,
				key.depth_settings, key.attachment_blend_settings, key.topology, key.render_pass);
		return pipeline.get();
	}

	PipelineCache::ComputePipelineKey::ComputePipelineKey()
	{
		compute_shader = nullptr;
	}

	bool PipelineCache::ComputePipelineKey::operator<(const ComputePipelineKey& other) const
	{
		return
			std::tie(compute_shader, pipeline_layout) <
			std::tie(other.compute_shader, other.pipeline_layout);
	}

	lz::ComputePipeline* PipelineCache::get_compute_pipeline(const ComputePipelineKey& key)
	{
		auto& pipeline = compute_pipeline_cache_[key];
		if (!pipeline)
			pipeline = std::make_unique<lz::ComputePipeline>(logical_device_, key.compute_shader, key.pipeline_layout);
		return pipeline.get();
	}
}
