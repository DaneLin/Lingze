#pragma once
#include <map>

#include "DescriptorSetCache.h"
#include "LingzeVK.h"
#include "Pipeline.h"
#include "VertexDeclaration.h"

namespace lz
{

	struct ShaderStageInfo
	{
		vk::ShaderStageFlagBits stage;
		vk::ShaderModule module;

		bool operator<(const ShaderStageInfo& other) const
		{
			return std::tie(stage, module) < std::tie(other.stage, other.module);
		}
	};

	class PipelineCache
	{
	public:
		PipelineCache(vk::Device logical_device, DescriptorSetCache* descriptor_set_cache);

		struct PipelineInfo
		{
			vk::PipelineLayout pipeline_layout;
			std::vector<vk::DescriptorSetLayout> descriptor_set_layouts;
		};

		PipelineInfo bind_graphics_pipeline(
			vk::CommandBuffer command_buffer,
			vk::RenderPass render_pass,
			lz::DepthSettings depth_settings,
			const std::vector<lz::BlendSettings>& attachment_blend_settings,
			const lz::VertexDeclaration& vertex_declaration,
			vk::PrimitiveTopology topology,
			const lz::ShaderProgram* shader_program);


		PipelineInfo bind_compute_pipeline(
			vk::CommandBuffer command_buffer,
			lz::Shader* compute_shader);

		void clear();

	private:
		struct PipelineLayoutKey
		{
			std::vector<vk::DescriptorSetLayout> set_layouts;

			bool operator <(const PipelineLayoutKey& other) const;
		};

		vk::UniquePipelineLayout create_pipeline_layout(
			const std::vector<vk::DescriptorSetLayout>& set_layouts/*, push constant ranges*/);

		vk::PipelineLayout get_pipeline_layout(const PipelineLayoutKey& key);

		

		struct GraphicsPipelineKey
		{
			GraphicsPipelineKey();

			std::vector<ShaderStageInfo> shader_stages;
			lz::VertexDeclaration vertex_decl;
			vk::PipelineLayout pipeline_layout;
			vk::Extent2D extent;
			vk::RenderPass render_pass;
			lz::DepthSettings depth_settings;
			std::vector<lz::BlendSettings> attachment_blend_settings;
			vk::PrimitiveTopology topology;

			bool operator <(const GraphicsPipelineKey& other) const;
		};

		lz::GraphicsPipeline* get_graphics_pipeline(const GraphicsPipelineKey& key);


		struct ComputePipelineKey
		{
			ComputePipelineKey();

			vk::ShaderModule compute_shader;
			vk::PipelineLayout pipeline_layout;

			bool operator <(const ComputePipelineKey& other) const;
		};

		lz::ComputePipeline* get_compute_pipeline(const ComputePipelineKey& key);

		std::map<GraphicsPipelineKey, std::unique_ptr<lz::GraphicsPipeline>> graphics_pipeline_cache_;
		std::map<ComputePipelineKey, std::unique_ptr<lz::ComputePipeline>> compute_pipeline_cache_;
		std::map<PipelineLayoutKey, vk::UniquePipelineLayout> pipeline_layout_cache_;
		lz::DescriptorSetCache* descriptor_set_cache_;

		vk::Device logical_device_;
	};
}
