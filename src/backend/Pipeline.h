#pragma once

#include "LingzeVK.h"
#include "VertexDeclaration.h"

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

		vk::CompareOp depthFunc; // Depth comparison function
		bool writeEnable; // Whether depth writing is enabled

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

		vk::PipelineColorBlendAttachmentState blendState; // Native Vulkan blend state
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
		vk::Pipeline GetHandle();

		// GetLayout: Returns the pipeline layout
		vk::PipelineLayout GetLayout();

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
		);

	private:
		vk::PipelineLayout pipelineLayout;
		vk::UniquePipeline pipeline;
		friend class Core;
	};


	class ComputePipeline
	{
	public:
		vk::Pipeline GetHandle();

		vk::PipelineLayout GetLayout();

		ComputePipeline(
			vk::Device logicalDevice,
			vk::ShaderModule computeShader,
			vk::PipelineLayout pipelineLayout
		);

	private:
		vk::PipelineLayout pipelineLayout;
		vk::UniquePipeline pipeline;
		friend class Core;
	};
}
