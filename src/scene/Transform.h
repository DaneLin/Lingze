#pragma once

#include "Component.h"
#include "glm/glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/transform.hpp"

namespace lz
{

/**
 * @brief The transform component.
 */
class Transform : public Component
{
  public:
	Transform(Entity *entity);

	// set the position
	void set_position(const glm::vec3 &position);

	// get the position
	const glm::vec3 &get_position() const
	{
		return position_;
	}

	// set the rotation
	void set_rotation(const glm::vec3 &rotation);

	// get the rotation
	const glm::vec3 &get_rotation() const
	{
		return rotation_;
	}

	// set the scale
	void set_scale(const glm::vec3 &scale);

	// get the scale
	const glm::vec3 &get_scale() const
	{
		return scale_;
	}

	// get the local matrix
	glm::mat4 get_local_matrix();

	// get the world matrix
	glm::mat4 get_world_matrix();

  private:
	glm::vec3 position_;            // the position
	glm::vec3 rotation_;            // the rotation
	glm::vec3 scale_;               // the scale
	glm::mat4 local_matrix_;        // the local matrix
	bool      matrix_dirty_;        // the matrix is dirty
};

}        // namespace lz