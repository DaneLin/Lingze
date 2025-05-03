#pragma once

#include "backend/App.h"

namespace lz
{
namespace application
{

// Define DemoApp class, inherits from App base class
class BasicShapeApp : public lz::App
{
  public:
	BasicShapeApp() :
	    lz::App("Lingze Basic Shape Example", 1280, 760)
	{
		// Clear the default instance extensions and set our own
		clear_instance_extensions();
		// Add additional device extensions if needed
		// Add required instance extensions
		add_instance_extension("VK_KHR_surface", true);
		add_instance_extension("VK_KHR_win32_surface", true);
		add_device_extension(VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME, true);
	}

	virtual ~BasicShapeApp() = default;

  protected:
	// Load scene
	virtual bool load_scene() override;

	// Create renderer
	virtual std::unique_ptr<lz::render::BaseRenderer> create_renderer() override;
};

}        // namespace application
}        // namespace lz