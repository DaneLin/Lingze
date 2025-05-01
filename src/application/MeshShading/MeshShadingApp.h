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
	    lz::App("Lingze Mesh Shading", 1280, 760)
	{
		mesh_shading_enabled_ = true;
	}

	virtual ~MeshShadingApp() override = default;

  protected:
	// Load scene
	virtual bool load_scene() override;

	virtual void render_ui() override;

	// Create renderer
	virtual std::unique_ptr<lz::render::BaseRenderer> create_renderer() override;

  private:
	bool mesh_shading_enabled_;
};

}        // namespace application
}        // namespace lz