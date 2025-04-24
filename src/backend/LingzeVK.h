#pragma once
// Define Win32 platform for Vulkan and prevent min/max macro definitions
#define VK_USE_PLATFORM_WIN32_KHR
#define NOMINMAX
#include <vulkan/vulkan.hpp>

// Include GLM for mathematics operations
#include <glm/glm.hpp>

// 移除TINYOBJLOADER_IMPLEMENTATION，避免多重定义
