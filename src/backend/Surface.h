#pragma once
#include "LingzeVK.h"

namespace lz
{
	// WindowDesc: Structure to store Win32 window information
	// - Used to create a Vulkan surface for rendering to a window
	struct WindowDesc
	{
		HINSTANCE h_instance;  // Win32 application instance handle
		HWND h_wnd;            // Win32 window handle
	};

	// CreateWin32Surface: Creates a Vulkan surface for rendering to a Win32 window
	// Parameters:
	//   - instance: Vulkan instance
	//   - desc: Window descriptor containing the Win32 handles
	// Returns: A unique handle to the created Vulkan surface
	static vk::UniqueSurfaceKHR create_win32_surface(const vk::Instance instance, const WindowDesc desc)
	{
		const auto& surface_create_info = vk::Win32SurfaceCreateInfoKHR()
			.setHwnd(desc.h_wnd)
			.setHinstance(desc.h_instance);

		return instance.createWin32SurfaceKHRUnique(surface_create_info);
	}
}
