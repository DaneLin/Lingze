#pragma once

#include "backend/App.h"

namespace lz
{
namespace application
{

// Define DemoApp class, inherits from App base class
class GpuDrivenApp : public lz::App
{
  public:
	GpuDrivenApp() :
	    lz::App("Lingze Gpu Driven Example", 1280, 760)
	{
		add_device_extension(VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME, true);
	}

	virtual ~GpuDrivenApp() override = default;

  protected:
	// Load scene
	virtual bool load_scene() override;

	virtual void render_ui() override;

	// Create renderer
	virtual std::unique_ptr<lz::render::BaseRenderer> create_renderer() override;

  private:
};

}        // namespace application
}        // namespace lz