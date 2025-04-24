#include "VertexDeclaration.h"

namespace lz
{
	void VertexDeclaration::add_vertex_input_binding(uint32_t buffer_binding, uint32_t stride)
	{
		const auto binding_desc = vk::VertexInputBindingDescription()
		                          .setBinding(buffer_binding)
		                          .setStride(stride)
		                          .setInputRate(vk::VertexInputRate::eVertex);
		binding_descriptors_.push_back(binding_desc);
	}

	void VertexDeclaration::add_vertex_attribute(uint32_t buffer_binding, uint32_t offset, AttribTypes attrib_type,
	                                             uint32_t shader_location)
	{
		const auto vertex_attribute = vk::VertexInputAttributeDescription()
		                              .setBinding(buffer_binding)
		                              .setFormat(convert_attrib_type_to_format(attrib_type))
		                              .setLocation(shader_location)
		                              .setOffset(offset);
		vertex_attributes_.push_back(vertex_attribute);
	}

	const std::vector<vk::VertexInputBindingDescription>& VertexDeclaration::get_binding_descriptors() const
	{
		return binding_descriptors_;
	}

	const std::vector<vk::VertexInputAttributeDescription>& VertexDeclaration::get_vertex_attributes() const
	{
		return vertex_attributes_;
	}

	bool VertexDeclaration::operator<(const VertexDeclaration& other) const
	{
		return std::tie(binding_descriptors_, vertex_attributes_) < std::tie(
			other.binding_descriptors_, other.vertex_attributes_);
	}

	vk::Format VertexDeclaration::convert_attrib_type_to_format(AttribTypes attrib_type)
	{
		switch (attrib_type)
		{
		case AttribTypes::eFloatType: return vk::Format::eR32Sfloat;
			break;
		case AttribTypes::eVec2: return vk::Format::eR32G32Sfloat;
			break;
		case AttribTypes::eVec3: return vk::Format::eR32G32B32Sfloat;
			break;
		case AttribTypes::eVec4: return vk::Format::eR32G32B32A32Sfloat;
			break;
		case AttribTypes::eU8Vec3: return vk::Format::eR8G8B8Unorm;
			break;
		case AttribTypes::eU8Vec4: return vk::Format::eR8G8B8A8Unorm;
			break;
		case AttribTypes::eI8Vec4: return vk::Format::eR8G8B8A8Snorm;
			break;
		case AttribTypes::eColor32: return vk::Format::eR8G8B8A8Unorm;
			break;
		}
		return vk::Format::eUndefined;
	}
}
