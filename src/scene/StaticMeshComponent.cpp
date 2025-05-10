#include "StaticMeshComponent.h"
#include "Entity.h"

namespace lz
{

StaticMeshComponent::StaticMeshComponent(Entity *entity) :
    Component(entity), mesh_(nullptr)
{
}

void StaticMeshComponent::set_mesh(LzMesh *mesh)
{
	mesh_ = mesh;
}

void StaticMeshComponent::set_material(const std::string &material_name)
{
	material_name_ = material_name;
}

}        // namespace lz