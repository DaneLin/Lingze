#pragma once
#include <map>

namespace lz
{
	class PipelineCache
	{
	public:
		PipelineCache(vk::Device _logicalDevice, DescriptorSetCache* _descriptorSetCache) :
			logicalDevice(_logicalDevice),
			descriptorSetCache(_descriptorSetCache)
		{
		}

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
			lz::ShaderProgram* shaderProgram)
		{
			GraphicsPipelineKey pipelineKey;
			pipelineKey.vertexShader = shaderProgram->vertexShader->GetModule()->GetHandle();
			pipelineKey.fragmentShader = shaderProgram->fragmentShader->GetModule()->GetHandle();
			pipelineKey.vertexDecl = vertexDeclaration;
			pipelineKey.depthSettings = depthSettings;
			pipelineKey.attachmentBlendSettings = attachmentBlendSettings;
			pipelineKey.topology = topology;

			PipelineInfo pipelineInfo;

			lz::Shader* targetShader = shaderProgram->vertexShader;

			PipelineLayoutKey pipelineLayoutKey;
			for (auto& setLayoutKey : shaderProgram->combinedDescriptorSetLayoutKeys)
			{
				pipelineLayoutKey.setLayouts.push_back(descriptorSetCache->GetDescriptorSetLayout(setLayoutKey));
			}

			pipelineKey.pipelineLayout = GetPipelineLayout(pipelineLayoutKey);

			pipelineKey.renderPass = renderPass;

			lz::GraphicsPipeline* pipeline = GetGraphicsPipeline(pipelineKey);

			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->GetHandle());

			pipelineInfo.pipelineLayout = pipeline->GetLayout();
			return pipelineInfo;
		}


		PipelineInfo BindComputePipeline(
			vk::CommandBuffer commandBuffer,
			lz::Shader* computeShader)
		{
			ComputePipelineKey pipelineKey;
			pipelineKey.computeShader = computeShader->GetModule()->GetHandle();

			PipelineInfo pipelineInfo;
			lz::Shader* targetShader = computeShader;

			PipelineLayoutKey pipelineLayoutKey;
			pipelineLayoutKey.setLayouts.resize(computeShader->GetSetsCount());
			for (size_t setIndex = 0; setIndex < pipelineLayoutKey.setLayouts.size(); setIndex++)
			{
				vk::DescriptorSetLayout setLayoutHandle = nullptr;
				auto computeSetInfo = computeShader->GetSetInfo(setIndex);
				if (!computeSetInfo->IsEmpty())
					setLayoutHandle = descriptorSetCache->GetDescriptorSetLayout(*computeSetInfo);

				pipelineLayoutKey.setLayouts[setIndex] = setLayoutHandle;
			}
			pipelineKey.pipelineLayout = GetPipelineLayout(pipelineLayoutKey);

			lz::ComputePipeline* pipeline = GetComputePipeline(pipelineKey);

			commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline->GetHandle());

			pipelineInfo.pipelineLayout = pipeline->GetLayout();
			return pipelineInfo;
		}

		void Clear()
		{
			this->computePipelineCache.clear();
			this->graphicsPipelineCache.clear();
			this->pipelineLayoutCache.clear();
		}

	private:
		struct PipelineLayoutKey
		{
			std::vector<vk::DescriptorSetLayout> setLayouts;

			bool operator <(const PipelineLayoutKey& other) const
			{
				return
					std::tie(setLayouts) < std::tie(other.setLayouts);
			}
		};

		vk::UniquePipelineLayout CreatePipelineLayout(
			const std::vector<vk::DescriptorSetLayout>& setLayouts/*, push constant ranges*/)
		{
			auto pipelineLayoutInfo = vk::PipelineLayoutCreateInfo()
			                          .setPushConstantRangeCount(0)
			                          .setPPushConstantRanges(nullptr)
			                          .setSetLayoutCount(uint32_t(setLayouts.size()))
			                          .setPSetLayouts(setLayouts.data());

			return logicalDevice.createPipelineLayoutUnique(pipelineLayoutInfo);
		}

		vk::PipelineLayout GetPipelineLayout(const PipelineLayoutKey& key)
		{
			auto& pipelineLayout = pipelineLayoutCache[key];
			if (!pipelineLayout)
				pipelineLayout = CreatePipelineLayout(key.setLayouts);
			return pipelineLayout.get();
		}

		struct GraphicsPipelineKey
		{
			GraphicsPipelineKey()
			{
				vertexShader = nullptr;
				fragmentShader = nullptr;
				renderPass = nullptr;
			}

			vk::ShaderModule vertexShader;
			vk::ShaderModule fragmentShader;
			lz::VertexDeclaration vertexDecl;
			vk::PipelineLayout pipelineLayout;
			vk::Extent2D extent;
			vk::RenderPass renderPass;
			lz::DepthSettings depthSettings;
			std::vector<lz::BlendSettings> attachmentBlendSettings;
			vk::PrimitiveTopology topology;

			bool operator <(const GraphicsPipelineKey& other) const
			{
				return
					std::tie(vertexShader, fragmentShader, vertexDecl, pipelineLayout, renderPass, depthSettings,
					         attachmentBlendSettings, topology) <
					std::tie(other.vertexShader, other.fragmentShader, other.vertexDecl, other.pipelineLayout,
					         other.renderPass, other.depthSettings, other.attachmentBlendSettings, topology);
			}
		};

		lz::GraphicsPipeline* GetGraphicsPipeline(const GraphicsPipelineKey& key)
		{
			auto& pipeline = graphicsPipelineCache[key];
			if (!pipeline)
				pipeline = std::unique_ptr<lz::GraphicsPipeline>(new lz::GraphicsPipeline(
					logicalDevice, key.vertexShader, key.fragmentShader, key.vertexDecl, key.pipelineLayout,
					key.depthSettings, key.attachmentBlendSettings, key.topology, key.renderPass));
			return pipeline.get();
		}


		struct ComputePipelineKey
		{
			ComputePipelineKey()
			{
				computeShader = nullptr;
			}

			vk::ShaderModule computeShader;
			vk::PipelineLayout pipelineLayout;

			bool operator <(const ComputePipelineKey& other) const
			{
				return
					std::tie(computeShader, pipelineLayout) <
					std::tie(other.computeShader, other.pipelineLayout);
			}
		};

		lz::ComputePipeline* GetComputePipeline(const ComputePipelineKey& key)
		{
			auto& pipeline = computePipelineCache[key];
			if (!pipeline)
				pipeline = std::unique_ptr<lz::ComputePipeline>(
					new lz::ComputePipeline(logicalDevice, key.computeShader, key.pipelineLayout));
			return pipeline.get();
		}

		std::map<GraphicsPipelineKey, std::unique_ptr<lz::GraphicsPipeline>> graphicsPipelineCache;
		std::map<ComputePipelineKey, std::unique_ptr<lz::ComputePipeline>> computePipelineCache;
		std::map<PipelineLayoutKey, vk::UniquePipelineLayout> pipelineLayoutCache;
		lz::DescriptorSetCache* descriptorSetCache;

		vk::Device logicalDevice;
	};
}
