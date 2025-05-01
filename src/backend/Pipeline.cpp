#include "Pipeline.h"

#include "PipelineCache.h"
#include "VertexDeclaration.h"

namespace lz
{
vk::Pipeline GraphicsPipeline::get_handle()
{
	return pipeline_.get();
}

vk::PipelineLayout GraphicsPipeline::get_layout() const
{
	return pipeline_layout_;
}

GraphicsPipeline::GraphicsPipeline(vk::Device logical_device, const std::vector<lz::ShaderStageInfo> &shader_stages,
                                   const lz::VertexDeclaration &vertex_decl, vk::PipelineLayout pipeline_layout, DepthSettings depth_settings,
                                   const std::vector<BlendSettings> &attachment_blend_settings, vk::PrimitiveTopology primitive_topology,
                                   vk::RenderPass render_pass)
{
	this->pipeline_layout_ = pipeline_layout;

	std::vector<vk::PipelineShaderStageCreateInfo> shader_stage_infos;
	for (auto &shader_stage : shader_stages)
	{
		auto shader_stage_create_info = vk::PipelineShaderStageCreateInfo()
		                                    .setStage(shader_stage.stage)
		                                    .setModule(shader_stage.module)
		                                    .setPName("main");
		shader_stage_infos.push_back(shader_stage_create_info);
	}

	// Configure vertex input state
	auto vertex_input_info = vk::PipelineVertexInputStateCreateInfo()
	                             .setVertexBindingDescriptionCount(
	                                 uint32_t(vertex_decl.get_binding_descriptors().size()))
	                             .setPVertexBindingDescriptions(vertex_decl.get_binding_descriptors().data())
	                             .setVertexAttributeDescriptionCount(
	                                 uint32_t(vertex_decl.get_vertex_attributes().size()))
	                             .setPVertexAttributeDescriptions(vertex_decl.get_vertex_attributes().data());

	// Configure input assembly state
	auto input_assembly_info = vk::PipelineInputAssemblyStateCreateInfo()
	                               .setTopology(primitive_topology)
	                               .setPrimitiveRestartEnable(false);

	// Configure rasterization state
	auto rasterization_state_info = vk::PipelineRasterizationStateCreateInfo()
	                                    .setDepthClampEnable(false)
	                                    .setPolygonMode(vk::PolygonMode::eFill)
	                                    .setLineWidth(1.0f)
	                                    .setCullMode(vk::CullModeFlagBits::eNone)
	                                    .setFrontFace(vk::FrontFace::eClockwise)
	                                    .setDepthBiasEnable(false);

	// Configure multisample state
	auto multisample_state_info = vk::PipelineMultisampleStateCreateInfo()
	                                  .setSampleShadingEnable(false)
	                                  .setRasterizationSamples(vk::SampleCountFlagBits::e1);

	// Configure color blend state for each attachment
	std::vector<vk::PipelineColorBlendAttachmentState> color_blend_attachment_states;
	for (const auto &blend_settings : attachment_blend_settings)
	{
		color_blend_attachment_states.push_back(blend_settings.blend_state);
	}

	auto color_blend_state_info = vk::PipelineColorBlendStateCreateInfo()
	                                  .setLogicOpEnable(false)
	                                  .setAttachmentCount(uint32_t(color_blend_attachment_states.size()))
	                                  .setPAttachments(color_blend_attachment_states.data());

	// Configure depth/stencil state
	auto depth_stencil_state = vk::PipelineDepthStencilStateCreateInfo()
	                               .setStencilTestEnable(false)
	                               .setDepthTestEnable(depth_settings.depth_func == vk::CompareOp::eAlways ? false : true)
	                               .setDepthCompareOp(depth_settings.depth_func)
	                               .setDepthWriteEnable(depth_settings.write_enable)
	                               .setDepthBoundsTestEnable(false);

	// Configure dynamic state (viewport and scissor)
	vk::DynamicState dynamic_states[]   = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
	auto             dynamic_state_info = vk::PipelineDynamicStateCreateInfo()
	                              .setDynamicStateCount(2)
	                              .setPDynamicStates(dynamic_states);

	auto viewport_state = vk::PipelineViewportStateCreateInfo()
	                          .setScissorCount(1)
	                          .setViewportCount(1);
	auto pipeline_create_info = vk::GraphicsPipelineCreateInfo()
	                                .setStageCount(uint32_t(shader_stage_infos.size()))
	                                .setPStages(shader_stage_infos.data())
	                                .setPVertexInputState(&vertex_input_info)
	                                .setPInputAssemblyState(&input_assembly_info)
	                                .setPRasterizationState(&rasterization_state_info)
	                                .setPViewportState(&viewport_state)
	                                .setPMultisampleState(&multisample_state_info)
	                                .setPDepthStencilState(&depth_stencil_state)
	                                .setPColorBlendState(&color_blend_state_info)
	                                .setPDynamicState(&dynamic_state_info)
	                                .setLayout(pipeline_layout)
	                                .setRenderPass(render_pass)
	                                .setSubpass(0)
	                                .setBasePipelineHandle(nullptr)        // use later
	                                .setBasePipelineIndex(-1);

	pipeline_ = logical_device.createGraphicsPipelineUnique(nullptr, pipeline_create_info).value;
}

vk::Pipeline ComputePipeline::get_handle()
{
	return pipeline_.get();
}

vk::PipelineLayout ComputePipeline::get_layout() const
{
	return pipeline_layout_;
}

ComputePipeline::ComputePipeline(vk::Device logical_device, vk::ShaderModule compute_shader,
                                 vk::PipelineLayout pipeline_layout)
{
	this->pipeline_layout_               = pipeline_layout;
	const auto compute_stage_create_info = vk::PipelineShaderStageCreateInfo()
	                                           .setStage(vk::ShaderStageFlagBits::eCompute)
	                                           .setModule(compute_shader)
	                                           .setPName("main");

	auto pipeline_create_info = vk::ComputePipelineCreateInfo()
	                                .setFlags(vk::PipelineCreateFlags())
	                                .setStage(compute_stage_create_info)
	                                .setLayout(pipeline_layout)
	                                .setBasePipelineHandle(nullptr)        // use later
	                                .setBasePipelineIndex(-1);

	pipeline_ = logical_device.createComputePipelineUnique(nullptr, pipeline_create_info).value;
}
}        // namespace lz
