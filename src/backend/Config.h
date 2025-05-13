#pragma once
// Define Win32 platform for Vulkan and prevent min/max macro definitions
#define VK_USE_PLATFORM_WIN32_KHR
#define NOMINMAX
#include <vulkan/vulkan.hpp>

