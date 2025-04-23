#pragma once

#include "LingzeVK.h"

namespace vk
{
	static bool operator <(const vk::VertexInputBindingDescription& desc0,
	                       const vk::VertexInputBindingDescription& desc1)
	{
		return std::tie(desc0.binding, desc0.inputRate, desc0.stride) < std::tie(
			desc1.binding, desc1.inputRate, desc1.stride);
	}

	static bool operator <(const vk::VertexInputAttributeDescription& desc0,
	                       const vk::VertexInputAttributeDescription& desc1)
	{
		return std::tie(desc0.binding, desc0.format, desc0.location, desc0.offset) < std::tie(
			desc1.binding, desc1.format, desc1.location, desc1.offset);
	}
}

namespace lz
{
	class VertexDeclaration
	{
	public:
		enum struct AttribTypes : uint8_t
		{
			floatType,
			vec2,
			vec3,
			vec4,
			u8vec3,
			u8vec4,
			i8vec4,
			color32
		};

		void AddVertexInputBinding(uint32_t bufferBinding, uint32_t stride);

		void AddVertexAttribute(uint32_t bufferBinding, uint32_t offset, AttribTypes attribType,
		                        uint32_t shaderLocation);

		const std::vector<vk::VertexInputBindingDescription>& GetBindingDescriptors() const;

		const std::vector<vk::VertexInputAttributeDescription>& GetVertexAttributes() const;

		bool operator<(const VertexDeclaration& other) const;

	private:
		static vk::Format ConvertAttribTypeToFormat(AttribTypes attribType);

		std::vector<vk::VertexInputBindingDescription> bindingDescriptors;
		std::vector<vk::VertexInputAttributeDescription> vertexAttributes;
	};
}
