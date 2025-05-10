#pragma once

#include "Component.h"
#include "scene/Mesh.h"
#include <string>

namespace lz
{

/**
 * @brief The mesh renderer component.
 */
class StaticMeshComponent : public Component
{
  public:
	StaticMeshComponent(Entity *entity);

	// set the mesh
	void set_mesh(Mesh *mesh);

	// get the mesh
	Mesh *get_mesh()
	{
		return mesh_;
	}

	// set the material
	void set_material(const std::string &material_name);

	// get the material name
	const std::string &get_material() const
	{
		return material_name_;
	}

  private:
	Mesh       *mesh_;                 // the mesh data
	std::string material_name_;        // the material name
};

}        // namespace lz