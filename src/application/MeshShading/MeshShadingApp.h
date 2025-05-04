#pragma once

#include "backend/App.h"

namespace lz
{
namespace application
{

// Define DemoApp class, inherits from App base class
class MeshShadingApp : public lz::App
{
  public:
	MeshShadingApp() :
	    lz::App("Lingze Mesh Shading Example", 1280, 760)
	{
		add_device_extension(VK_EXT_MESH_SHADER_EXTENSION_NAME, true);
		add_device_extension(VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME, true);
	}

	virtual ~MeshShadingApp() override = default;

  protected:
	// Load scene
	virtual bool load_scene() override;

	virtual void render_ui() override;

	// Create renderer
	virtual std::unique_ptr<lz::render::BaseRenderer> create_renderer() override;

  private:
	bool mesh_shading_enabled_ = false;
};

}        // namespace application
}        // namespace lz