#include "MeshShadingApp.h"

#include "application/EntryPoint.h"
#include "MeshShadingRenderer.h"

namespace lz::application
{
	bool MeshShadingApp::load_scene()
	{
		std::string config_file_name = SCENE_DIR "CubeScene.json";
		//std::string config_file_name = SCENE_DIR "SponzaScene.json";

		bool result = load_scene_from_file(config_file_name, lz::Scene::GeometryTypes::eTriangles);

		if (result && core_->mesh_shader_supported())
		{
			scene_->create_global_buffers();
			scene_->create_draw_buffer();
		}

		return result;
	}

	void MeshShadingApp::render_ui()
	{
		App::render_ui();

		ImGui::Begin("Demo features", 0, ImGuiWindowFlags_NoScrollbar);
		{
			ImGui::Checkbox("Enable Mesh Shading", &mesh_shading_enabled_);
		}
		ImGui::End();
	}

	std::unique_ptr<lz::render::BaseRenderer> MeshShadingApp::create_renderer()
	{
		return std::make_unique<lz::render::MeshShadingRenderer>(core_.get());
	}
}

// Use macro to generate main function
LINGZE_MAIN(lz::application::MeshShadingApp)