#include "MeshShadingApp.h"

#include "MeshShadingRenderer.h"
#include "application/EntryPoint.h"

#include "scene/Entity.h"
#include "scene/EntityScene.h"
#include "scene/LzMesh.h"
#include "scene/MeshLoader.h"
#include "scene/StaticMeshComponent.h"
#include "scene/Transform.h"

namespace lz::application
{
void MeshShadingApp::prepare_render_context()
{
	// std::string config_file_name = SCENE_DIR "SingleHornbug.json";
	// std::string config_file_name = SCENE_DIR "SingleKitten.json";
	//std::string config_file_name = SCENE_DIR "SponzaScene.json";
	std::string config_file_name = DATA_DIR "glTF-Sample-Assets/Models/Sponza/glTF/Sponza.gltf";
	load_scene_from_file(config_file_name, lz::JsonScene::GeometryTypes::eTriangles);

#if NEW_MESH_PATH
	auto   loader = LzMeshLoader::get_loader(config_file_name);
	LzMesh mesh   = loader->load(config_file_name);

	Scene scene;
	auto  entity        = scene.create_entity("Sponza");
	auto  mesh_renderer = entity->add_component<StaticMeshComponent>();
	mesh_renderer->set_mesh(&mesh);

	render_context_->collect_draw_commands(&scene);
	render_context_->create_gpu_resources();
	render_context_->build_meshlet_data();
	render_context_->create_meshlet_buffer();
#endif
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