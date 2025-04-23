#pragma once
#include <map>

#include "DescriptorSetCache.h"
#include "LingzeVK.h"
#include "Pipeline.h"
#include "VertexDeclaration.h"

namespace lz
{


	class PipelineCache
	{
	public:
		PipelineCache(vk::Device _logicalDevice, DescriptorSetCache* _descriptorSetCache);

		struct PipelineInfo
		{
			vk::PipelineLayout pipelineLayout;
			std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
		};

		PipelineInfo BindGraphicsPipeline(
			vk::CommandBuffer commandBuffer,
			vk::RenderPass renderPass,
			lz::DepthSettings depthSettings,
			const std::vector<lz::BlendSettings>& attachmentBlendSettings,
			lz::VertexDeclaration vertexDeclaration,
			vk::PrimitiveTopology topology,
			lz::ShaderProgram* shaderProgram);


		PipelineInfo BindComputePipeline(
			vk::CommandBuffer commandBuffer,
			lz::Shader* computeShader);

		void Clear();

	private:
		struct PipelineLayoutKey
		{
			std::vector<vk::DescriptorSetLayout> setLayouts;

			bool operator <(const PipelineLayoutKey& other) const;
		};

		vk::UniquePipelineLayout CreatePipelineLayout(
			const std::vector<vk::DescriptorSetLayout>& setLayouts/*, push constant ranges*/);

		vk::PipelineLayout GetPipelineLayout(const PipelineLayoutKey& key);

		struct GraphicsPipelineKey
		{
			GraphicsPipelineKey();

			vk::ShaderModule vertexShader;
			vk::ShaderModule fragmentShader;
			lz::VertexDeclaration vertexDecl;
			vk::PipelineLayout pipelineLayout;
			vk::Extent2D extent;
			vk::RenderPass renderPass;
			lz::DepthSettings depthSettings;
			std::vector<lz::BlendSettings> attachmentBlendSettings;
			vk::PrimitiveTopology topology;

			bool operator <(const GraphicsPipelineKey& other) const;
		};

		lz::GraphicsPipeline* GetGraphicsPipeline(const GraphicsPipelineKey& key);


		struct ComputePipelineKey
		{
			ComputePipelineKey();

			vk::ShaderModule computeShader;
			vk::PipelineLayout pipelineLayout;

			bool operator <(const ComputePipelineKey& other) const;
		};

		lz::ComputePipeline* GetComputePipeline(const ComputePipelineKey& key);

		std::map<GraphicsPipelineKey, std::unique_ptr<lz::GraphicsPipeline>> graphicsPipelineCache;
		std::map<ComputePipelineKey, std::unique_ptr<lz::ComputePipeline>> computePipelineCache;
		std::map<PipelineLayoutKey, vk::UniquePipelineLayout> pipelineLayoutCache;
		lz::DescriptorSetCache* descriptorSetCache;

		vk::Device logicalDevice;
	};

	
}
