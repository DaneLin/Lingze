#pragma once

#include <memory>

namespace lz
{
// forward declaration
class Entity;

/**
 * @brief The base class of the component.
 */
class Component
{
  public:
    Component(Entity* entity);
    virtual ~Component() = default;
    
    Entity* get_entity() { return entity_; }
    
  protected:
    Entity* entity_;  // the entity that owns this component
};

} // namespace lz 