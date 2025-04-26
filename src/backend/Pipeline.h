#pragma once

#include "LingzeVK.h"
#include "VertexDeclaration.h"
#include "ShaderProgram.h"

namespace lz
{
	class Core;

	// DepthSettings: Structure for configuring depth testing and writing
	// - Controls depth test function and whether depth writing is enabled
	struct DepthSettings
	{
		// DepthTest: Creates depth settings with depth testing enabled
		// Returns: Depth settings configured for standard depth testing
		static DepthSettings enabled()
		{
			DepthSettings settings;
			settings.depth_func = vk::CompareOp::eLess;
			settings.write_enable = true;
			return settings;
		}

		// Disabled: Creates depth settings with depth testing disabled
		// Returns: Depth settings configured with depth testing disabled
		static DepthSettings disabled()
		{
			DepthSettings settings;
			settings.depth_func = vk::CompareOp::eAlways;
			settings.write_enable = false;
			return settings;
		}

		vk::CompareOp depth_func; // Depth comparison function
		bool write_enable; // Whether depth writing is enabled

		// Comparison operator for container ordering
		bool operator<(const DepthSettings& other) const
		{
			return std::tie(depth_func, write_enable) < std::tie(other.depth_func, other.write_enable);
		}
	};

	// BlendSettings: Structure for configuring color blending
	// - Controls how fragment colors are blended with the framebuffer
	struct BlendSettings
	{
		// Opaque: Creates blend settings for opaque rendering (no blending)
		// Returns: Blend settings configured for opaque rendering
		static BlendSettings opaque()
		{
			BlendSettings blend_settings;
			blend_settings.blend_state = vk::PipelineColorBlendAttachmentState()
			                             .setColorWriteMask(
				                             vk::ColorComponentFlagBits::eA | vk::ColorComponentFlagBits::eR |
				                             vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB)
			                             .setBlendEnable(false);
			return blend_settings;
		}

		// Add: Creates blend settings for additive blending
		// Returns: Blend settings configured for additive blending
		static BlendSettings add()
		{
			BlendSettings blend_settings;
			blend_settings.blend_state = vk::PipelineColorBlendAttachmentState()
			                             .setBlendEnable(true)
			                             .setAlphaBlendOp(vk::BlendOp::eAdd)
			                             .setColorBlendOp(vk::BlendOp::eAdd)
			                             .setColorWriteMask(
				                             vk::ColorComponentFlagBits::eA | vk::ColorComponentFlagBits::eR |
				                             vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB)
			                             .setSrcColorBlendFactor(vk::BlendFactor::eOne)
			                             .setDstColorBlendFactor(vk::BlendFactor::eOne);
			return blend_settings;
		}

		// Mixed: Creates blend settings for mixed blending
		// Returns: Blend settings configured for mixed color and alpha blending
		static BlendSettings mixed()
		{
			BlendSettings blend_settings;
			blend_settings.blend_state = vk::PipelineColorBlendAttachmentState()
			                             .setBlendEnable(true)
			                             .setAlphaBlendOp(vk::BlendOp::eAdd)
			                             .setColorBlendOp(vk::BlendOp::eAdd)
			                             .setColorWriteMask(
				                             vk::ColorComponentFlagBits::eA | vk::ColorComponentFlagBits::eR |
				                             vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB)
			                             .setSrcColorBlendFactor(vk::BlendFactor::eOne)
			                             .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha);
			return blend_settings;
		}

		// AlphaBlend: Creates blend settings for standard alpha blending
		// Returns: Blend settings configured for standard alpha blending
		static BlendSettings alpha_blend()
		{
			BlendSettings blend_settings;
			blend_settings.blend_state = vk::PipelineColorBlendAttachmentState()
			                             .setBlendEnable(true)
			                             .setAlphaBlendOp(vk::BlendOp::eAdd)
			                             .setColorBlendOp(vk::BlendOp::eAdd)
			                             .setColorWriteMask(
				                             vk::ColorComponentFlagBits::eA | vk::ColorComponentFlagBits::eR |
				                             vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB)
			                             .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
			                             .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha);
			return blend_settings;
		}

		// Comparison operator for container ordering
		bool operator <(const BlendSettings& other) const
		{
			return
				std::tie(blend_state.blendEnable, blend_state.alphaBlendOp, blend_state.colorBlendOp,
				         blend_state.srcColorBlendFactor, blend_state.dstColorBlendFactor) <
				std::tie(other.blend_state.blendEnable, other.blend_state.alphaBlendOp, other.blend_state.colorBlendOp,
				         other.blend_state.srcColorBlendFactor, other.blend_state.dstColorBlendFactor);
		}

		vk::PipelineColorBlendAttachmentState blend_state; // Native Vulkan blend state
	};

	// GraphicsPipeline: Class for managing Vulkan graphics pipelines
	// - Encapsulates the state for rendering operations
	// - Includes vertex input, blending, depth testing, and shader stages
	class GraphicsPipeline
	{
	public:
		// Enumeration of predefined blend modes
		enum struct BlendModes : uint8_t
		{
			eOpaque
		};

		// Enumeration of predefined depth/stencil modes
		enum struct DepthStencilModes : uint8_t
		{
			eDepthNone,
			eDepthLess
		};

		// GetHandle: Returns the native Vulkan pipeline handle
		vk::Pipeline get_handle();

		// GetLayout: Returns the pipeline layout
		vk::PipelineLayout get_layout() const;

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
			vk::Device logical_device,
			vk::ShaderModule vertex_shader,
			vk::ShaderModule fragment_shader,
			const lz::VertexDeclaration& vertex_decl,
			vk::PipelineLayout pipeline_layout,
			DepthSettings depth_settings,
			const std::vector<BlendSettings>& attachment_blend_settings,
			vk::PrimitiveTopology primitive_topology,
			vk::RenderPass render_pass
		);

		GraphicsPipeline(
			vk::Device logical_device,
			ShaderProgram& shader_program,
			const lz::VertexDeclaration& vertex_decl,
			vk::PipelineLayout pipeline_layout,
			DepthSettings depth_settings,
			const std::vector<BlendSettings>& attachment_blend_settings,
			vk::PrimitiveTopology primitive_topology,
			vk::RenderPass render_pass
		);

	private:
		vk::PipelineLayout pipeline_layout_;
		vk::UniquePipeline pipeline_;
		friend class Core;
	};


	class ComputePipeline
	{
	public:
		vk::Pipeline get_handle();

		vk::PipelineLayout get_layout() const;

		ComputePipeline(
			vk::Device logical_device,
			vk::ShaderModule compute_shader,
			vk::PipelineLayout pipeline_layout
		);

	private:
		vk::PipelineLayout pipeline_layout_;
		vk::UniquePipeline pipeline_;
		friend class Core;
	};
}
