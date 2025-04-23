#pragma once

#include "LingzeVK.h"

namespace lz
{
	// Sampler: Class for managing Vulkan samplers
	// - Encapsulates sampling parameters for reading from textures in shaders
	// - Controls filtering, address modes, and other sampling behavior
	class Sampler
	{
	public:
		// GetHandle: Returns the native Vulkan sampler handle
		vk::Sampler GetHandle() const;

		// Comparison operator for container ordering
		bool operator <(const Sampler& other) const;

		// Constructor: Creates a new sampler with the specified parameters
		// Parameters:
		// - logicalDevice: Logical device for creating the sampler
		// - addressMode: How texture coordinates outside [0,1] range are handled
		// - minMagFilterType: Filtering mode for minification and magnification
		// - mipFilterType: Filtering mode between mipmap levels
		// - useComparison: Whether to use comparison mode for shadow sampling
		// - borderColor: Color used for border address mode
		Sampler(vk::Device logicalDevice, vk::SamplerAddressMode addressMode, vk::Filter minMagFilterType,
		        vk::SamplerMipmapMode mipFilterType, bool useComparison = false,
		        vk::BorderColor borderColor = vk::BorderColor());

	private:
		vk::UniqueSampler samplerHandle;  // Native Vulkan sampler handle
	};
}
