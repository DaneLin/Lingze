#pragma once

#include "backend/App.h"

namespace lz
{
namespace application
{

// Define DemoApp class, inherits from App base class
class SimpleMeshShadingApp : public lz::App
{
  public:
	SimpleMeshShadingApp() :
	    lz::App("Lingze Simple Mesh Shading", 1280, 760)
	{}

	virtual ~SimpleMeshShadingApp() = default;

  protected:
	// Load scene
	virtual bool load_scene() override;

	// Create renderer
	virtual std::unique_ptr<lz::render::BaseRenderer> create_renderer() override;
};

}        // namespace application
}        // namespace lz