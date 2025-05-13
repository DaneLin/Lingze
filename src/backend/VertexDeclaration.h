#pragma once

#include "Config.h"

namespace vk
{
static bool operator<(const vk::VertexInputBindingDescription &desc0,
                      const vk::VertexInputBindingDescription &desc1)
{
	return std::tie(desc0.binding, desc0.inputRate, desc0.stride) < std::tie(desc1.binding, desc1.inputRate, desc1.stride);
}

static bool operator<(const vk::VertexInputAttributeDescription &desc0,
                      const vk::VertexInputAttributeDescription &desc1)
{
	return std::tie(desc0.binding, desc0.format, desc0.location, desc0.offset) < std::tie(desc1.binding, desc1.format, desc1.location, desc1.offset);
}
}        // namespace vk

namespace lz
{
class VertexDeclaration
{
  public:
	enum struct AttribTypes : uint8_t
	{
		eFloatType,
		eVec2,
		eVec3,
		eVec4,
		eU8Vec3,
		eU8Vec4,
		eI8Vec4,
		eColor32
	};

	void add_vertex_input_binding(uint32_t buffer_binding, uint32_t stride);

	void add_vertex_attribute(uint32_t buffer_binding, uint32_t offset, AttribTypes attrib_type, uint32_t shader_location);

	const std::vector<vk::VertexInputBindingDescription> &get_binding_descriptors() const;

	const std::vector<vk::VertexInputAttributeDescription> &get_vertex_attributes() const;

	bool operator<(const VertexDeclaration &other) const;

  private:
	static vk::Format convert_attrib_type_to_format(AttribTypes attrib_type);

	std::vector<vk::VertexInputBindingDescription>   binding_descriptors_;
	std::vector<vk::VertexInputAttributeDescription> vertex_attributes_;
};
}        // namespace lz
