#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include <glm/gtx/transform.hpp>    

namespace lz
{
/**
 * @brief Camera class for handling camera transformations
 */
class Camera
{
public:
	Camera();
	~Camera() = default;

	/**
	 * @brief Get the transformation matrix for the camera
	 * @return The transformation matrix
	 */
	glm::mat4 get_transform_matrix() const;

	/**
	 * @brief Get the view matrix for the camera
	 * @return The view matrix for rendering
	 */
	glm::mat4 get_view_matrix() const;

	/**
	 * @brief Get the projection matrix for the camera
	 * @return The projection matrix for rendering
	 */
	glm::mat4 get_projection_matrix() const;

	/**
	 * @brief Set the perspective projection parameters
	 * @param fov Field of view in radians
	 * @param aspect_ratio Aspect ratio (width/height)
	 * @param near_plane Near clipping plane distance
	 * @param far_plane Far clipping plane distance
	 */
	void set_perspective(float fov, float aspect_ratio, float near_plane, float far_plane);

	/**
	 * @brief Look at a specific point
	 * @param target The point to look at
	 * @param up World up vector, default is (0,1,0)
	 */
	void look_at(const glm::vec3& target, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f));

	/**
	 * @brief Get the near plane distance
	 * @return The near plane distance
	 */
	float get_near_plane() const;
	

	/**
	 * @brief Get the far plane distance
	 * @return The far plane distance
	 */
	float get_far_plane() const;

	// Camera position
	glm::vec3 pos;
	
	// Camera rotation angles (in radians)
	float vert_angle;  // Vertical angle (pitch)
	float hor_angle;   // Horizontal angle (yaw)

private:
	// Projection parameters
	float fov_;           // Field of view (in radians)
	float aspect_ratio_;  // Aspect ratio (width/height)
	float near_plane_;    // Near clipping plane distance
	float far_plane_;     // Far clipping plane distance
	
	// Flag to indicate if projection matrix needs to be recalculated
	mutable bool projection_dirty_;
	// Cached projection matrix
	mutable glm::mat4 projection_matrix_;
};

}        // namespace lz
