#pragma once

namespace lz
{
	class Core;

	// DepthSettings: Structure for configuring depth testing and writing
	// - Controls depth test function and whether depth writing is enabled
	struct DepthSettings
	{
		// DepthTest: Creates depth settings with depth testing enabled
		// Returns: Depth settings configured for standard depth testing
		static DepthSettings DepthTest()
		{
			DepthSettings settings;
			settings.depthFunc = vk::CompareOp::eLess;
			settings.writeEnable = true;
			return settings;
		}

		// Disabled: Creates depth settings with depth testing disabled
		// Returns: Depth settings configured with depth testing disabled
		static DepthSettings Disabled()
		{
			DepthSettings settings;
			settings.depthFunc = vk::CompareOp::eAlways;
			settings.writeEnable = false;
			return settings;
		}

		vk::CompareOp depthFunc;  // Depth comparison function
		bool writeEnable;         // Whether depth writing is enabled

		// Comparison operator for container ordering
		bool operator<(const DepthSettings& other) const
		{
			return std::tie(depthFunc, writeEnable) < std::tie(other.depthFunc, other.writeEnable);
		}
	};

	// BlendSettings: Structure for configuring color blending
	// - Controls how fragment colors are blended with the framebuffer
	struct BlendSettings
	{
		// Opaque: Creates blend settings for opaque rendering (no blending)
		// Returns: Blend settings configured for opaque rendering
		static BlendSettings Opaque()
		{
			BlendSettings blendSettings;
			blendSettings.blendState = vk::PipelineColorBlendAttachmentState()
			                           .setColorWriteMask(
				                           vk::ColorComponentFlagBits::eA | vk::ColorComponentFlagBits::eR |
				                           vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB)
			                           .setBlendEnable(false);
			return blendSettings;
		}

		// Add: Creates blend settings for additive blending
		// Returns: Blend settings configured for additive blending
		static BlendSettings Add()
		{
			BlendSettings blendSettings;
			blendSettings.blendState = vk::PipelineColorBlendAttachmentState()
			                           .setBlendEnable(true)
			                           .setAlphaBlendOp(vk::BlendOp::eAdd)
			                           .setColorBlendOp(vk::BlendOp::eAdd)
			                           .setColorWriteMask(
				                           vk::ColorComponentFlagBits::eA | vk::ColorComponentFlagBits::eR |
				                           vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB)
			                           .setSrcColorBlendFactor(vk::BlendFactor::eOne)
			                           .setDstColorBlendFactor(vk::BlendFactor::eOne);
			return blendSettings;
		}

		// Mixed: Creates blend settings for mixed blending
		// Returns: Blend settings configured for mixed color and alpha blending
		static BlendSettings Mixed()
		{
			BlendSettings blendSettings;
			blendSettings.blendState = vk::PipelineColorBlendAttachmentState()
			                           .setBlendEnable(true)
			                           .setAlphaBlendOp(vk::BlendOp::eAdd)
			                           .setColorBlendOp(vk::BlendOp::eAdd)
			                           .setColorWriteMask(
				                           vk::ColorComponentFlagBits::eA | vk::ColorComponentFlagBits::eR |
				                           vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB)
			                           .setSrcColorBlendFactor(vk::BlendFactor::eOne)
			                           .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha);
			return blendSettings;
		}

		// AlphaBlend: Creates blend settings for standard alpha blending
		// Returns: Blend settings configured for standard alpha blending
		static BlendSettings AlphaBlend()
		{
			BlendSettings blendSettings;
			blendSettings.blendState = vk::PipelineColorBlendAttachmentState()
			                           .setBlendEnable(true)
			                           .setAlphaBlendOp(vk::BlendOp::eAdd)
			                           .setColorBlendOp(vk::BlendOp::eAdd)
			                           .setColorWriteMask(
				                           vk::ColorComponentFlagBits::eA | vk::ColorComponentFlagBits::eR |
				                           vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB)
			                           .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
			                           .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha);
			return blendSettings;
		}

		// Comparison operator for container ordering
		bool operator <(const BlendSettings& other) const
		{
			return
				std::tie(blendState.blendEnable, blendState.alphaBlendOp, blendState.colorBlendOp,
				         blendState.srcColorBlendFactor, blendState.dstColorBlendFactor) <
				std::tie(other.blendState.blendEnable, other.blendState.alphaBlendOp, other.blendState.colorBlendOp,
				         other.blendState.srcColorBlendFactor, other.blendState.dstColorBlendFactor);
		}

		vk::PipelineColorBlendAttachmentState blendState;  // Native Vulkan blend state
	};

	// GraphicsPipeline: Class for managing Vulkan graphics pipelines
	// - Encapsulates the state for rendering operations
	// - Includes vertex input, blending, depth testing, and shader stages
	class GraphicsPipeline
	{
	public:
		// Enumeration of predefined blend modes
		enum struct BlendModes
		{
			Opaque
		};

		// Enumeration of predefined depth/stencil modes
		enum struct DepthStencilModes
		{
			DepthNone,
			DepthLess
		};

		// GetHandle: Returns the native Vulkan pipeline handle
		vk::Pipeline GetHandle()
		{
			return pipeline.get();
		}

		// GetLayout: Returns the pipeline layout
		vk::PipelineLayout GetLayout()
		{
			return pipelineLayout;
		}

		// Constructor: Creates a new graphics pipeline with the specified parameters
		// Parameters:
		// - logicalDevice: Logical device for creating the pipeline
		// - vertexShader: Vertex shader module
		// - fragmentShader: Fragment shader module
		// - vertexDecl: Vertex declaration describing input vertex data
		// - pipelineLayout: Pipeline layout for uniform and push constant access
		// - depthSettings: Depth testing and writing configuration
		// - attachmentBlendSettings: Blend settings for each color attachment
		// - primitiveTopology: Type of primitives to render
		// - renderPass: Render pass the pipeline is compatible with
		GraphicsPipeline(
			vk::Device logicalDevice,
			vk::ShaderModule vertexShader,
			vk::ShaderModule fragmentShader,
			const lz::VertexDeclaration& vertexDecl,
			vk::PipelineLayout pipelineLayout,
			DepthSettings depthSettings,
			const std::vector<BlendSettings>& attachmentBlendSettings,
			vk::PrimitiveTopology primitiveTopology,
			vk::RenderPass renderPass
		)
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

	private:
		vk::PipelineLayout pipelineLayout;
		vk::UniquePipeline pipeline;
		friend class Core;
	};

	class ComputePipeline
	{
	public:
		vk::Pipeline GetHandle()
		{
			return pipeline.get();
		}

		vk::PipelineLayout GetLayout()
		{
			return pipelineLayout;
		}

		ComputePipeline(
			vk::Device logicalDevice,
			vk::ShaderModule computeShader,
			vk::PipelineLayout pipelineLayout
		)
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

	private:
		vk::PipelineLayout pipelineLayout;
		vk::UniquePipeline pipeline;
		friend class Core;
	};
}
