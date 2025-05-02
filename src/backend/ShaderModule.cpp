#include "ShaderModule.h"

namespace lz
{
vk::ShaderModule ShaderModule::get_handle()
{
	return shader_module_.get();
}

ShaderModule::ShaderModule(vk::Device device, const std::vector<uint32_t> &bytecode)
{
	init(device, bytecode);
}

void ShaderModule::init(vk::Device device, const std::vector<uint32_t> &bytecode)
{
	const auto shader_module_create_info = vk::ShaderModuleCreateInfo()
	                                           .setCodeSize(bytecode.size() * sizeof(uint32_t))
	                                           .setPCode(bytecode.data());
	this->shader_module_ = device.createShaderModuleUnique(shader_module_create_info);
}
}        // namespace lz
