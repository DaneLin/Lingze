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
		vk::Sampler get_handle() const;

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
		Sampler(vk::Device logical_device, vk::SamplerAddressMode address_mode, vk::Filter min_mag_filter_type,
		        vk::SamplerMipmapMode mip_filter_type, bool use_comparison = false,
		        vk::BorderColor border_color = vk::BorderColor());

	private:
		vk::UniqueSampler sampler_handle_; // Native Vulkan sampler handle
	};
}
