#pragma once
#include <cstdint>

namespace lz
{
	// QueueFamilyIndices: Structure to store queue family indices for graphics and presentation
	// - Used to configure device queues and swapchain image sharing
	// - Graphics queue is used for rendering commands
	// - Present queue is used for presenting images to the display
	struct QueueFamilyIndices
	{
		uint32_t graphics_family_index;  // Index of the graphics queue family
		uint32_t present_family_index;   // Index of the presentation queue family
	};
}
