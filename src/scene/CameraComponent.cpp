#include "CameraComponent.h"
#include "Entity.h"
#include "Scene.h"
#include "Transform.h"

namespace lz
{

CameraComponent::CameraComponent(Entity *owner) :
    Component(owner), camera_(std::make_shared<Camera>())
{
	initialize();
}

CameraComponent::~CameraComponent()
{
}

void CameraComponent::initialize()
{
}

std::shared_ptr<Camera> CameraComponent::get_camera() const
{
	return camera_;
}

void CameraComponent::set_position(const glm::vec3 &position)
{
	camera_->pos = position;
}

void CameraComponent::set_rotation(float vertical_angle, float horizontal_angle)
{
	camera_->vert_angle = vertical_angle;
	camera_->hor_angle  = horizontal_angle;
}

void CameraComponent::set_as_main_camera(Scene *scene)
{
	if (scene && camera_)
	{
		scene->set_main_camera(camera_);
	}
}

}        // namespace lz