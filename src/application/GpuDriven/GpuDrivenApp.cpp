#include "GpuDrivenApp.h"

#include "GpuDrivenRenderer.h"
#include "application/EntryPoint.h"

#include "scene/Entity.h"
#include "scene/EntityScene.h"
#include "scene/LzMesh.h"
#include "scene/MeshLoader.h"
#include "scene/StaticMeshComponent.h"
#include "scene/Transform.h"

namespace lz::application
{
void GpuDrivenApp::prepare_render_context()
{
	// Option 1: Load JSON scene
	// std::string config_file_name = SCENE_DIR "SingleHornbug.json";
	// std::string config_file_name = SCENE_DIR "SingleKitten.json";
	// std::string config_file_name = SCENE_DIR "SponzaScene.json";
	//std::string config_file_name = SCENE_DIR "CubeScene.json";

	// Option 2: Load GLTF scene directly
	std::string config_file_name = DATA_DIR "glTF-Sample-Assets/Models/Sponza/glTF/Sponza.gltf";

	// You can still use SceneGraph for other purposes if needed
	// std::unique_ptr<SceneGraph> scene_graph = std::make_unique<SceneGraph>(core_.get());
	// scene_graph->load_model(DATA_DIR "glTF-Sample-Assets/Models/Sponza/glTF/Sponza.gltf");

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

	// load_scene_from_file(config_file_name, lz::JsonScene::GeometryTypes::eTriangles);
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