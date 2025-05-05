#pragma once

#include "glm/glm.hpp"
#include <glm/gtx/transform.hpp>    

namespace lz
{
/**
 * @brief Camera class for handling camera transformations
 */
struct Camera
{
	Camera()
	{
		pos        = glm::vec3(0.0f);
		vert_angle = 0.0f;
		hor_angle  = 0.0f;
	}

	/**
	 * @brief Get the transformation matrix for the camera
	 * @return The transformation matrix
	 */
	glm::mat4 get_transform_matrix() const
	{
		return glm::translate(pos) * glm::rotate(hor_angle, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::rotate(vert_angle, glm::vec3(1.0f, 0.0f, 0.0f));
	}

	glm::vec3 pos;
	float     vert_angle;
	float     hor_angle;
};

}        // namespace lz
