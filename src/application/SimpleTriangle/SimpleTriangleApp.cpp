#include "SimpleTriangleApp.h"
#include "../EntryPoint.h"
#include "SimpleRenderer.h"

namespace lz::application
{

bool SimpleMeshShadingApp::load_scene()
{
	return true;
}

std::unique_ptr<lz::render::BaseRenderer> SimpleMeshShadingApp::create_renderer()
{
	// Use simple renderer
	return std::make_unique<lz::render::SimpleRenderer>(core_.get());
}
}        // namespace lz::application

// Use macro to generate main function
LINGZE_MAIN(lz::application::SimpleMeshShadingApp)