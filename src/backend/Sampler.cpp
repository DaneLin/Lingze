#include "Sampler.h"

namespace lz
{
	vk::Sampler Sampler::GetHandle() const
	{
		return samplerHandle.get();
	}

	bool Sampler::operator<(const Sampler& other) const
	{
		return std::tie(samplerHandle.get()) < std::tie(other.samplerHandle.get());
	}

	Sampler::Sampler(vk::Device logicalDevice, vk::SamplerAddressMode addressMode, vk::Filter minMagFilterType,
		vk::SamplerMipmapMode mipFilterType, bool useComparison, vk::BorderColor borderColor)
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
}
