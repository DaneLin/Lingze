#pragma once
namespace lz
{
	// Sampler: Class for managing Vulkan samplers
	// - Encapsulates sampling parameters for reading from textures in shaders
	// - Controls filtering, address modes, and other sampling behavior
	class Sampler
	{
	public:
		// GetHandle: Returns the native Vulkan sampler handle
		vk::Sampler GetHandle() const
		{
			return samplerHandle.get();
		}

		// Comparison operator for container ordering
		bool operator <(const Sampler& other) const
		{
			return std::tie(samplerHandle.get()) < std::tie(other.samplerHandle.get());
		}

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
		        vk::BorderColor borderColor = vk::BorderColor())
		{
			// Configure sampler creation parameters
			auto samplerCreateInfo = vk::SamplerCreateInfo()
			                         .setAddressModeU(addressMode)     // Address mode for U coordinate
			                         .setAddressModeV(addressMode)     // Address mode for V coordinate
			                         .setAddressModeW(addressMode)     // Address mode for W coordinate
			                         .setAnisotropyEnable(false)       // Disable anisotropic filtering
			                         .setCompareEnable(useComparison)  // Enable/disable comparison mode
			                         .setCompareOp(useComparison ? vk::CompareOp::eLessOrEqual : vk::CompareOp::eAlways)
			                         .setMagFilter(minMagFilterType)   // Magnification filter
			                         .setMinFilter(minMagFilterType)   // Minification filter
			                         .setMaxLod(1e7f)                  // Maximum level of detail
			                         .setMinLod(0.0f)                  // Minimum level of detail
			                         .setMipmapMode(mipFilterType)     // Mipmap filtering mode
			                         .setUnnormalizedCoordinates(false)// Use normalized coordinates
			                         .setBorderColor(borderColor);     // Border color

			// Create the sampler
			samplerHandle = logicalDevice.createSamplerUnique(samplerCreateInfo);
		}

	private:
		vk::UniqueSampler samplerHandle;  // Native Vulkan sampler handle
	};
}
