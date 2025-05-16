#include "MeshShadingApp.h"

#include "MeshShadingRenderer.h"
#include "application/EntryPoint.h"

#include "scene/Entity.h"
#include "scene/Mesh.h"
#include "scene/MeshLoader.h"
#include "scene/Scene.h"
#include "scene/StaticMeshComponent.h"
#include "scene/Transform.h"

namespace lz::application
{
void MeshShadingApp::prepare_render_context()
{
	Scene scene;

	auto  Sponza = scene.create_entity("Sponza");
	Mesh  mesh   = MeshLoaderManager::get_instance().load(GLTF_DIR "Sponza/glTF/Sponza.gltf");
	Sponza->get_transform()->set_rotation(glm::vec3(0.0f, 90.0f, 0.0f));
	Sponza->add_component<StaticMeshComponent>()->set_mesh(&mesh);

	Mesh buddha_mesh = MeshLoaderManager::get_instance().load(DATA_DIR "Meshes/buddha.obj");

	auto buddha = scene.create_entity("Buddha");
	buddha->get_transform()->set_scale(glm::vec3(0.5f, 0.5f, 0.5f));
	buddha->add_component<StaticMeshComponent>()->set_mesh(&buddha_mesh);

	auto buddha1 = scene.create_entity("Buddha1");
	buddha1->get_transform()->set_scale(glm::vec3(0.1f, 0.1f, 0.1f));
	buddha1->get_transform()->set_position(glm::vec3(-3.0f, 0.0f, 4.0f));
	buddha1->add_component<StaticMeshComponent>()->set_mesh(&buddha_mesh);

	render_context_->collect_draw_commands(&scene);
	render_context_->build_meshlet_data();
	render_context_->create_gpu_resources();
	render_context_->create_meshlet_buffer();
}

void MeshShadingApp::render_ui()
{
	App::render_ui();

	auto camera = scene_->get_main_camera();
	// Camera position
	ImGui::Begin("Camera Position");
	ImGui::Text("Camera Position: %f, %f, %f", camera->pos.x, camera->pos.y, camera->pos.z);
	ImGui::End();
}

std::unique_ptr<lz::render::BaseRenderer> MeshShadingApp::create_renderer()
{
	return std::make_unique<lz::render::MeshShadingRenderer>(core_.get());
}
}        // namespace lz::application

// Use macro to generate main function
LINGZE_MAIN(lz::application::MeshShadingApp)