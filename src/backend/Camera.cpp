#include "Camera.h"
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>

namespace lz
{

Camera::Camera() :
    pos(0.0f), vert_angle(0.0f), hor_angle(0.0f), fov_(glm::radians(45.0f)), aspect_ratio_(16.0f / 9.0f), near_plane_(0.1f), far_plane_(10000.0f), projection_dirty_(true), projection_matrix_(1.0f)
{
}

glm::mat4 Camera::get_transform_matrix() const
{
	return glm::translate(pos) * glm::rotate(hor_angle, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::rotate(vert_angle, glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::mat4 Camera::get_view_matrix() const
{
	return glm::inverse(get_transform_matrix());
}

glm::mat4 Camera::get_projection_matrix() const
{
	if (projection_dirty_)
	{
		projection_matrix_ = glm::perspectiveZO(fov_, aspect_ratio_, near_plane_, far_plane_) *
		                     glm::scale(glm::vec3(1.0f, -1.0f, -1.0f));

		projection_dirty_ = false;
	}

	return projection_matrix_;
}

void Camera::set_perspective(float fov, float aspect_ratio, float near_plane, float far_plane)
{
	fov_              = fov;
	aspect_ratio_     = aspect_ratio;
	near_plane_       = near_plane;
	far_plane_        = far_plane;
	projection_dirty_ = true;
}

void Camera::look_at(const glm::vec3 &target, const glm::vec3 &up)
{
	const glm::vec3 direction = glm::normalize(target - pos);

	hor_angle  = atan2(direction.x, direction.z);
	vert_angle = asin(direction.y);
}

float Camera::get_near_plane() const
{
	return near_plane_;
}

float Camera::get_far_plane() const
{
	return far_plane_;
}
}        // namespace lz
