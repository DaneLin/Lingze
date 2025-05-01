#include "SimpleMeshShadingApp.h"
#include "../EntryPoint.h"
#include "SimpleMeshShadingRenderer.h"

namespace lz::application
{

bool SimpleMeshShadingApp::load_scene()
{
	// Demo application can use different scenes
	// std::string config_file_name = std::string(SCENE_DIR) + "CubeScene.json";  // Can also use other scenes
	// lz::Scene::GeometryTypes geo_type = lz::Scene::GeometryTypes::eTriangles;

	return true;
}

std::unique_ptr<lz::render::BaseRenderer> SimpleMeshShadingApp::create_renderer()
{
	// Use simple renderer
	return std::make_unique<lz::render::SimpleMeshShadingRenderer>(core_.get());
}
}        // namespace lz::application

// Use macro to generate main function
LINGZE_MAIN(lz::application::SimpleMeshShadingApp)