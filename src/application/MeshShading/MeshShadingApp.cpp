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
	Mesh mesh = MeshLoaderManager::get_instance().load(GLTF_DIR "Sponza/glTF/Sponza.gltf");

	Scene scene;
	auto  entity = scene.create_entity("Sponza");
	entity->add_component<StaticMeshComponent>()->set_mesh(&mesh);

	render_context_->collect_draw_commands(&scene);
	render_context_->create_gpu_resources();
	render_context_->build_meshlet_data();
	render_context_->create_meshlet_buffer();
}

void MeshShadingApp::render_ui()
{
	App::render_ui();
}

std::unique_ptr<lz::render::BaseRenderer> MeshShadingApp::create_renderer()
{
	return std::make_unique<lz::render::MeshShadingRenderer>(core_.get());
}
}        // namespace lz::application

// Use macro to generate main function
LINGZE_MAIN(lz::application::MeshShadingApp)