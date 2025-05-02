#include "SimpleMeshShadingApp.h"
#include "../EntryPoint.h"
#include "SimpleMeshShadingRenderer.h"

namespace lz::application
{
std::unique_ptr<lz::render::BaseRenderer> SimpleMeshShadingApp::create_renderer()
{
	// Use simple renderer
	return std::make_unique<lz::render::SimpleMeshShadingRenderer>(core_.get());
}
}        // namespace lz::application

// Use macro to generate main function
LINGZE_MAIN(lz::application::SimpleMeshShadingApp)