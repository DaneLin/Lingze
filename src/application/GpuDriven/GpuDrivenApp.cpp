#include "GpuDrivenApp.h"

#include "GpuDrivenRenderer.h"
#include "scene/SceneGraph.h"
#include "application/EntryPoint.h"

namespace lz::application
{
bool GpuDrivenApp::load_scene()
{
	// Option 1: Load JSON scene 
	// std::string config_file_name = SCENE_DIR "SingleHornbug.json";
	// std::string config_file_name = SCENE_DIR "SingleKitten.json";
	// std::string config_file_name = SCENE_DIR "SponzaScene.json";
	// std::string config_file_name = SCENE_DIR "CubeScene.json";

	// Option 2: Load GLTF scene directly
	std::string config_file_name = DATA_DIR "glTF-Sample-Assets/Models/Sponza/glTF/Sponza.gltf";
	
	// You can still use SceneGraph for other purposes if needed
	//std::unique_ptr<SceneGraph> scene_graph = std::make_unique<SceneGraph>(core_.get());
	//scene_graph->load_model(DATA_DIR "glTF-Sample-Assets/Models/Sponza/glTF/Sponza.gltf");
	
	return load_scene_from_file(config_file_name, lz::JsonScene::GeometryTypes::eTriangles);
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