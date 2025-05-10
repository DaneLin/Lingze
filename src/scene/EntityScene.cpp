#include "EntityScene.h"
#include "Entity.h"

namespace lz
{

Scene::Scene()
{
}

Scene::~Scene()
{
    // clear all entities
    root_entities_.clear();
    all_entities_.clear();
}

std::shared_ptr<Entity> Scene::create_entity(const std::string& name)
{
    auto entity = std::make_shared<Entity>(name);
    root_entities_.push_back(entity);
    all_entities_.push_back(entity);
    return entity;
}

void Scene::update(float delta_time)
{
    // add the scene update logic here, such as physics simulation
}

} // namespace lz 