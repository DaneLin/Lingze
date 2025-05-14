#include "Sampler.h"

namespace lz
{
vk::Sampler Sampler::get_handle() const
{
	return sampler_handle_.get();
}

bool Sampler::operator<(const Sampler &other) const
{
	return std::tie(sampler_handle_.get()) < std::tie(other.sampler_handle_.get());
}

Sampler::Sampler(vk::Device logical_device, vk::SamplerAddressMode address_mode, vk::Filter min_mag_filter_type,
                 vk::SamplerMipmapMode mip_filter_type, vk::SamplerReductionModeEXT reduction_mode, bool use_comparison, vk::BorderColor border_color)
{
	// Configure sampler creation parameters
	auto sampler_create_info = vk::SamplerCreateInfo()
	                                     .setAddressModeU(address_mode)           // Address mode for U coordinate
	                                     .setAddressModeV(address_mode)           // Address mode for V coordinate
	                                     .setAddressModeW(address_mode)           // Address mode for W coordinate
	                                     .setAnisotropyEnable(false)              // Disable anisotropic filtering
	                                     .setCompareEnable(use_comparison)        // Enable/disable comparison mode
	                                     .setCompareOp(use_comparison ? vk::CompareOp::eLessOrEqual : vk::CompareOp::eAlways)
	                                     .setMagFilter(min_mag_filter_type)        // Magnification filter
	                                     .setMinFilter(min_mag_filter_type)        // Minification filter
	                                     .setMaxLod(1e7f)                          // Maximum level of detail
	                                     .setMinLod(0.0f)                          // Minimum level of detail
	                                     .setMipmapMode(mip_filter_type)           // Mipmap filtering mode
	                                     .setUnnormalizedCoordinates(false)        // Use normalized coordinates
	                                     .setBorderColor(border_color);            // Border color

	auto reduction_mode_info = vk::SamplerReductionModeCreateInfoEXT();
	if (reduction_mode != vk::SamplerReductionModeEXT::eWeightedAverage)
	{
		reduction_mode_info.setReductionMode(reduction_mode);
		sampler_create_info.setPNext(&reduction_mode_info);
	}

	// Create the sampler
	sampler_handle_ = logical_device.createSamplerUnique(sampler_create_info);
}
Sampler::Sampler(vk::Device logical_device, vk::SamplerCreateInfo create_info)
{
	sampler_handle_ = logical_device.createSamplerUnique(create_info);
}

}        // namespace lz
