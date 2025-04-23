#include "Pipeline.h"

#include "VertexDeclaration.h"

namespace lz
{
	vk::Pipeline GraphicsPipeline::GetHandle()
	{
		return pipeline.get();
	}

	vk::PipelineLayout GraphicsPipeline::GetLayout()
	{
		return pipelineLayout;
	}

	GraphicsPipeline::GraphicsPipeline(vk::Device logicalDevice, vk::ShaderModule vertexShader,
	                                   vk::ShaderModule fragmentShader, const lz::VertexDeclaration& vertexDecl,
	                                   vk::PipelineLayout pipelineLayout,
	                                   DepthSettings depthSettings,
	                                   const std::vector<BlendSettings>& attachmentBlendSettings,
	                                   vk::PrimitiveTopology primitiveTopology, vk::RenderPass renderPass)
	{
		this->pipelineLayout = pipelineLayout;

		// Configure vertex shader stage
		auto vertexStageCreateInfo = vk::PipelineShaderStageCreateInfo()
		                             .setStage(vk::ShaderStageFlagBits::eVertex)
		                             .setModule(vertexShader)
		                             .setPName("main");

		// Configure fragment shader stage
		auto fragmentStageCreateInfo = vk::PipelineShaderStageCreateInfo()
		                               .setStage(vk::ShaderStageFlagBits::eFragment)
		                               .setModule(fragmentShader)
		                               .setPName("main");

		vk::PipelineShaderStageCreateInfo shaderStageInfos[] = {vertexStageCreateInfo, fragmentStageCreateInfo};

		// Configure vertex input state
		auto vertexInputInfo = vk::PipelineVertexInputStateCreateInfo()
		                       .setVertexBindingDescriptionCount(
			                       uint32_t(vertexDecl.GetBindingDescriptors().size()))
		                       .setPVertexBindingDescriptions(vertexDecl.GetBindingDescriptors().data())
		                       .setVertexAttributeDescriptionCount(
			                       uint32_t(vertexDecl.GetVertexAttributes().size()))
		                       .setPVertexAttributeDescriptions(vertexDecl.GetVertexAttributes().data());

		// Configure input assembly state
		auto inputAssemblyInfo = vk::PipelineInputAssemblyStateCreateInfo()
		                         .setTopology(primitiveTopology)
		                         .setPrimitiveRestartEnable(false);

		// Configure rasterization state
		auto rasterizationStateInfo = vk::PipelineRasterizationStateCreateInfo()
		                              .setDepthClampEnable(false)
		                              .setPolygonMode(vk::PolygonMode::eFill)
		                              .setLineWidth(1.0f)
		                              .setCullMode(vk::CullModeFlagBits::eNone)
		                              .setFrontFace(vk::FrontFace::eClockwise)
		                              .setDepthBiasEnable(false);

		// Configure multisample state
		auto multisampleStateInfo = vk::PipelineMultisampleStateCreateInfo()
		                            .setSampleShadingEnable(false)
		                            .setRasterizationSamples(vk::SampleCountFlagBits::e1);

		// Configure color blend state for each attachment
		std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachmentStates;
		for (const auto& blendSettings : attachmentBlendSettings)
		{
			colorBlendAttachmentStates.push_back(blendSettings.blendState);
		}

		auto colorBlendStateInfo = vk::PipelineColorBlendStateCreateInfo()
		                           .setLogicOpEnable(false)
		                           .setAttachmentCount(uint32_t(colorBlendAttachmentStates.size()))
		                           .setPAttachments(colorBlendAttachmentStates.data());

		// Configure depth/stencil state
		auto depthStencilState = vk::PipelineDepthStencilStateCreateInfo()
		                         .setStencilTestEnable(false)
		                         .setDepthTestEnable(depthSettings.depthFunc == vk::CompareOp::eAlways
			                                             ? false
			                                             : true)
		                         .setDepthCompareOp(depthSettings.depthFunc)
		                         .setDepthWriteEnable(depthSettings.writeEnable)
		                         .setDepthBoundsTestEnable(false);

		// Configure dynamic state (viewport and scissor)
		vk::DynamicState dynamicStates[] = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
		auto dynamicStateInfo = vk::PipelineDynamicStateCreateInfo()
		                        .setDynamicStateCount(2)
		                        .setPDynamicStates(dynamicStates);

		auto viewportState = vk::PipelineViewportStateCreateInfo()
		                     .setScissorCount(1)
		                     .setViewportCount(1);
		auto pipelineCreateInfo = vk::GraphicsPipelineCreateInfo()
		                          .setStageCount(2)
		                          .setPStages(shaderStageInfos)
		                          .setPVertexInputState(&vertexInputInfo)
		                          .setPInputAssemblyState(&inputAssemblyInfo)
		                          .setPRasterizationState(&rasterizationStateInfo)
		                          .setPViewportState(&viewportState)
		                          .setPMultisampleState(&multisampleStateInfo)
		                          .setPDepthStencilState(&depthStencilState)
		                          .setPColorBlendState(&colorBlendStateInfo)
		                          .setPDynamicState(&dynamicStateInfo)
		                          .setLayout(pipelineLayout)
		                          .setRenderPass(renderPass)
		                          .setSubpass(0)
		                          .setBasePipelineHandle(nullptr) //use later
		                          .setBasePipelineIndex(-1);

		pipeline = logicalDevice.createGraphicsPipelineUnique(nullptr, pipelineCreateInfo).value;
	}


	vk::Pipeline ComputePipeline::GetHandle()
	{
		return pipeline.get();
	}

	vk::PipelineLayout ComputePipeline::GetLayout()
	{
		return pipelineLayout;
	}

	ComputePipeline::ComputePipeline(vk::Device logicalDevice, vk::ShaderModule computeShader,
	                                 vk::PipelineLayout pipelineLayout)
	{
		this->pipelineLayout = pipelineLayout;
		auto computeStageCreateInfo = vk::PipelineShaderStageCreateInfo()
		                              .setStage(vk::ShaderStageFlagBits::eCompute)
		                              .setModule(computeShader)
		                              .setPName("main");

		auto viewportState = vk::PipelineViewportStateCreateInfo()
		                     .setScissorCount(1)
		                     .setViewportCount(1);

		auto pipelineCreateInfo = vk::ComputePipelineCreateInfo()
		                          .setFlags(vk::PipelineCreateFlags())
		                          .setStage(computeStageCreateInfo)
		                          .setLayout(pipelineLayout)
		                          .setBasePipelineHandle(nullptr) //use later
		                          .setBasePipelineIndex(-1);

		pipeline = logicalDevice.createComputePipelineUnique(nullptr, pipelineCreateInfo).value;
	}
}
