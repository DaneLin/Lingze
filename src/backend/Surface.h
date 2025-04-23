#pragma once
#include "LingzeVK.h"

namespace lz
{
	// WindowDesc: Structure to store Win32 window information
	// - Used to create a Vulkan surface for rendering to a window
	struct WindowDesc
	{
		HINSTANCE hInstance;  // Win32 application instance handle
		HWND hWnd;            // Win32 window handle
	};

	// CreateWin32Surface: Creates a Vulkan surface for rendering to a Win32 window
	// Parameters:
	//   - instance: Vulkan instance
	//   - desc: Window descriptor containing the Win32 handles
	// Returns: A unique handle to the created Vulkan surface
	static vk::UniqueSurfaceKHR CreateWin32Surface(vk::Instance instance, WindowDesc desc)
	{
		const auto& surfaceCreateInfo = vk::Win32SurfaceCreateInfoKHR()
			.setHwnd(desc.hWnd)
			.setHinstance(desc.hInstance);

		return instance.createWin32SurfaceKHRUnique(surfaceCreateInfo);
	}
}
