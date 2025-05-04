#include "GpuDrivenApp.h"

#include "GpuDrivenRenderer.h"
#include "application/EntryPoint.h"

namespace lz::application
{
bool GpuDrivenApp::load_scene()
{
	// std::string config_file_name = SCENE_DIR "SingleHornbug.json";
	// std::string config_file_name = SCENE_DIR "SingleKitten.json";
	std::string config_file_name = SCENE_DIR "SponzaScene.json";
	//std::string config_file_name = SCENE_DIR "CubeScene.json";
	return load_scene_from_file(config_file_name, lz::Scene::GeometryTypes::eTriangles);
}

void GpuDrivenApp::render_ui()
{
	App::render_ui();
}

std::unique_ptr<lz::render::BaseRenderer> GpuDrivenApp::create_renderer()
{
	return std::make_unique<lz::render::GpuDrivenRenderer>(core_.get());
}
}        // namespace lz::application

// Use macro to generate main function
LINGZE_MAIN(lz::application::GpuDrivenApp)