#include "MeshShadingApp.h"

#include "MeshShadingRenderer.h"
#include "application/EntryPoint.h"

namespace lz::application
{
bool MeshShadingApp::load_scene()
{
	// std::string config_file_name = SCENE_DIR "SingleHornbug.json";
	// std::string config_file_name = SCENE_DIR "SingleKitten.json";
	std::string config_file_name = SCENE_DIR "SponzaScene.json";
	bool        result           = load_scene_from_file(config_file_name, lz::Scene::GeometryTypes::eTriangles);

	if (result)
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

		if (ImGui::Checkbox("Enable Mesh Shading", &mesh_shading_enabled_))
		{
			dynamic_cast<lz::render::MeshShadingRenderer *>(renderer_.get())->set_mesh_shading_enable_flag(mesh_shading_enabled_);
		}	
	}
	ImGui::End();
}

std::unique_ptr<lz::render::BaseRenderer> MeshShadingApp::create_renderer()
{
	return std::make_unique<lz::render::MeshShadingRenderer>(core_.get());
}
}        // namespace lz::application

// Use macro to generate main function
LINGZE_MAIN(lz::application::MeshShadingApp)