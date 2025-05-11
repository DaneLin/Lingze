#pragma once

#include <memory>
#include <string>
#include <vector>

namespace lz
{

class Entity;
class Camera;

/**
 * @brief The scene class based on entities.
 */
class Scene
{
  public:
	Scene();
	~Scene();

	// create an entity
	std::shared_ptr<Entity> create_entity(const std::string &name = "Entity");

	// get the root entities
	const std::vector<std::shared_ptr<Entity>> &get_root_entities() const
	{
		return root_entities_;
	}

	// update the scene
	void update(float delta_time);

	void                                        set_main_camera(std::shared_ptr<Camera> camera);
	std::shared_ptr<Camera>                     get_main_camera() const;
	void                                        add_camera(std::shared_ptr<Camera> camera);
	const std::vector<std::shared_ptr<Camera>> &get_cameras() const;

  private:
	std::vector<std::shared_ptr<Entity>> root_entities_;
	std::vector<std::shared_ptr<Entity>> all_entities_;

	std::vector<std::shared_ptr<Camera>> cameras_;
	std::shared_ptr<Camera>              main_camera_;
};

}        // namespace lz