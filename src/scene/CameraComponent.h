#pragma once

#include "../backend/Camera.h"
#include "Component.h"
#include <memory>

namespace lz
{

class Scene;

/**
 * @brief Camera component for entities
 */
class CameraComponent : public Component
{
  public:
	CameraComponent(Entity *owner);
	~CameraComponent();

	void initialize();

	std::shared_ptr<Camera> get_camera() const;

	void set_position(const glm::vec3 &position);
	void set_rotation(float vertical_angle, float horizontal_angle);

	void set_as_main_camera(Scene *scene);

  private:
	std::shared_ptr<Camera> camera_;
};

}        // namespace lz