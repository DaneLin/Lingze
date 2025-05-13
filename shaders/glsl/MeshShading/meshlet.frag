#version 450

#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "mesh.h"
#include "../common_math.h"

layout(location = 0) out vec4 outColor;

layout (location = 0) in vec4 color;
layout (location = 1) in flat uint material_index;
layout (location = 2) in vec2 texcoord;

layout(set = 0, binding = 4) readonly buffer MaterialParametersBuffer
{
	MaterialParameters material_parameters[];
};

// bindless texture
layout(set = BINDLESS_SET_ID, binding = BINDLESS_TEXTURE_BINDING) uniform sampler2D textures[];

void main()
{
#if DEBUG
	outColor = color;
	return;
#endif
	MaterialParameters params = material_parameters[material_index];
	outColor = texture(textures[params.diffuse_texture_index], texcoord)  * params.base_color_factor;

}