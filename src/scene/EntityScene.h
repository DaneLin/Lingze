#pragma once

#include <memory>
#include <string>
#include <vector>

namespace lz
{

class Entity;

/**
 * @brief The scene class based on entities.
 */
class Scene
{
  public:
    Scene();
    ~Scene();
    
    // create an entity
    std::shared_ptr<Entity> create_entity(const std::string& name = "Entity");
    
    // get the root entities
    const std::vector<std::shared_ptr<Entity>>& get_root_entities() const { return root_entities_; }
    
    // update the scene
    void update(float delta_time);
    
  private:
    std::vector<std::shared_ptr<Entity>> root_entities_; // the root entities
    std::vector<std::shared_ptr<Entity>> all_entities_;  // the all entities
};

} // namespace lz 