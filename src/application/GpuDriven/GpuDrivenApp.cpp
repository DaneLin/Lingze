#include "GpuDrivenApp.h"

#include "GpuDrivenRenderer.h"
#include "application/EntryPoint.h"

#include "scene/Entity.h"
#include "scene/Mesh.h"
#include "scene/MeshLoader.h"
#include "scene/Scene.h"
#include "scene/StaticMeshComponent.h"
#include "scene/Transform.h"

namespace lz::application
{
void GpuDrivenApp::prepare_render_context()
{
	Scene scene;

	Mesh mesh   = MeshLoaderManager::get_instance().load(GLTF_DIR "Sponza/glTF/Sponza.gltf");
	auto entity = scene.create_entity("Sponza");
	entity->get_transform()->set_rotation(glm::vec3(0.0f, 90.0f, 90.0f));
	entity->add_component<StaticMeshComponent>()->set_mesh(&mesh);

	/*Mesh buddha_mesh = MeshLoaderManager::get_instance().load(DATA_DIR "Meshes/buddha.obj");
	auto buddha      = scene.create_entity("Buddha");
	buddha->get_transform()->set_scale(glm::vec3(0.5f, 0.5f, 0.5f));
	buddha->add_component<StaticMeshComponent>()->set_mesh(&buddha_mesh);*/

	render_context_->collect_draw_commands(&scene);
	render_context_->build_meshlet_data();
	render_context_->create_gpu_resources();
	render_context_->create_meshlet_buffer();
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