#version 450

layout(location = 0) out vec4 outColor;

layout (location = 0) in vec4 color;

layout(set = 1, binding = 11) uniform sampler2D textures[];

void main()
{
	outColor = color;
}