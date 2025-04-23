#pragma once
#include <string>
#include <fstream>

namespace lz
{
	class Core;

	// ShaderModule: Class for managing Vulkan shader modules
	// - Encapsulates a compiled shader in SPIR-V format
	// - Provides access to the native Vulkan shader module handle
	class ShaderModule
	{
	public:
		// GetHandle: Returns the native Vulkan shader module handle
		vk::ShaderModule GetHandle()
		{
			return shaderModule.get();
		}

		// Constructor: Creates a new shader module from SPIR-V bytecode
		// Parameters:
		// - device: Logical device for creating the shader module
		// - bytecode: SPIR-V bytecode as a vector of 32-bit words
		ShaderModule(vk::Device device, const std::vector<uint32_t>& bytecode)
		{
			Init(device, bytecode);
		}

	private:
		// Init: Internal initialization method
		// Parameters:
		// - device: Logical device for creating the shader module
		// - bytecode: SPIR-V bytecode as a vector of 32-bit words
		void Init(vk::Device device, const std::vector<uint32_t>& bytecode)
		{
			auto shaderModuleCreateInfo = vk::ShaderModuleCreateInfo()
			                              .setCodeSize(bytecode.size() * sizeof(uint32_t))
			                              .setPCode(bytecode.data());
			this->shaderModule = device.createShaderModuleUnique(shaderModuleCreateInfo);
		}

		vk::UniqueShaderModule shaderModule;  // Native Vulkan shader module handle
		friend class Core;
	};
}
