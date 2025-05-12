#pragma once
// Define Win32 platform for Vulkan and prevent min/max macro definitions
#define VK_USE_PLATFORM_WIN32_KHR
#define NOMINMAX
#include <vulkan/vulkan.hpp>

constexpr uint32_t k_bindless_descriptor_set_index = 1;
constexpr uint32_t k_bindless_texture_binding      = 11;
constexpr uint32_t k_max_bindless_resources        = 1024;

constexpr uint32_t k_max_common_resources = 1024;

