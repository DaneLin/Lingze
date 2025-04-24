#include "BasicShapeApp.h"

#include "EntryPoint.h"
#include "render/renderers/BasicShapeRenderer.h"

namespace lz
{
	namespace application
	{
		bool BasicShapeApp::load_scene()
		{
			std::string config_file_name = std::string(SCENE_DIR) + "CubeScene.json";
			//std::string config_file_name = std::string(SCENE_DIR) + "SponzaScene.json";
			lz::Scene::GeometryTypes geo_type = lz::Scene::GeometryTypes::eTriangles;

			return load_scene_from_file(config_file_name, geo_type);
		}

		std::unique_ptr<lz::render::BaseRenderer> BasicShapeApp::create_renderer()
		{
			return std::make_unique<lz::render::BasicShapeRenderer>(core_.get());
		}
	}
}

// Use macro to generate main function
LINGZE_MAIN(lz::application::BasicShapeApp)
