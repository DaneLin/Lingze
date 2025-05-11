#include "Scene.h"
#include "../backend/Camera.h"
#include "Entity.h"
#include <algorithm>

namespace lz
{

Scene::Scene() :
    main_camera_(nullptr)
{
}

Scene::~Scene()
{
	root_entities_.clear();
	all_entities_.clear();
	cameras_.clear();
	main_camera_ = nullptr;
}

std::shared_ptr<Entity> Scene::create_entity(const std::string &name)
{
	auto entity = std::make_shared<Entity>(name);
	root_entities_.push_back(entity);
	all_entities_.push_back(entity);
	return entity;
}

void Scene::update(float delta_time)
{
	cameras_.erase(
	    std::remove_if(cameras_.begin(), cameras_.end(),
	                   [](const std::shared_ptr<Camera> &camera) {
		                   return camera.use_count() <= 1;
	                   }),
	    cameras_.end());

	if (main_camera_ && main_camera_.use_count() <= 1)
	{
		main_camera_ = nullptr;

		if (!cameras_.empty())
		{
			main_camera_ = cameras_[0];
		}
	}
}

void Scene::set_main_camera(std::shared_ptr<Camera> camera)
{
	if (camera && std::find(cameras_.begin(), cameras_.end(), camera) == cameras_.end())
	{
		cameras_.push_back(camera);
	}

	main_camera_ = camera;
}

std::shared_ptr<Camera> Scene::get_main_camera() const
{
	return main_camera_;
}

void Scene::add_camera(std::shared_ptr<Camera> camera)
{
	if (camera && std::find(cameras_.begin(), cameras_.end(), camera) == cameras_.end())
	{
		cameras_.push_back(camera);
	}
}

const std::vector<std::shared_ptr<Camera>> &Scene::get_cameras() const
{
	return cameras_;
}

}        // namespace lz