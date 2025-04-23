#include "ShaderModule.h"

namespace lz
{
	vk::ShaderModule ShaderModule::GetHandle()
	{
		return shaderModule.get();
	}

	ShaderModule::ShaderModule(vk::Device device, const std::vector<uint32_t>& bytecode)
	{
		Init(device, bytecode);
	}

	void ShaderModule::Init(vk::Device device, const std::vector<uint32_t>& bytecode)
	{
		auto shaderModuleCreateInfo = vk::ShaderModuleCreateInfo()
		                              .setCodeSize(bytecode.size() * sizeof(uint32_t))
		                              .setPCode(bytecode.data());
		this->shaderModule = device.createShaderModuleUnique(shaderModuleCreateInfo);
	}
}
