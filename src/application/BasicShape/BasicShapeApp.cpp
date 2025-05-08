#include "BasicShapeApp.h"

#include "../EntryPoint.h"
#include "BasicShapeRenderer.h"

namespace lz
{
namespace application
{
bool BasicShapeApp::load_scene()
{
	// Standard JSON scene loading
	//std::string config_file_name = SCENE_DIR "CubeScene.json";
	
	// Direct GLTF loading example
	std::string config_file_name = DATA_DIR "glTF-Sample-Assets/Models/Cube/glTF/Cube.gltf";
	
	lz::JsonScene::GeometryTypes geo_type = lz::JsonScene::GeometryTypes::eTriangles;

	return load_scene_from_file(config_file_name, geo_type);
}

std::unique_ptr<lz::render::BaseRenderer> BasicShapeApp::create_renderer()
{
	return std::make_unique<lz::render::BasicShapeRenderer>(core_.get());
}
}        // namespace application
}        // namespace lz

// Use macro to generate main function
LINGZE_MAIN(lz::application::BasicShapeApp)
