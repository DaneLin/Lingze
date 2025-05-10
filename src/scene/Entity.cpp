#include "Entity.h"
#include "Transform.h"
#include "Component.h"
#include <algorithm>
#include <cassert>

namespace lz
{

// initialize the static member
uint64_t Entity::next_id_ = 0;

Entity::Entity(const std::string& name)
    : id_(next_id_++), name_(name), parent_(nullptr)
{
    // each entity automatically creates a Transform component
    transform_ = add_component<Transform>();
}

Entity::~Entity()
{
    // clear the children entities
    children_.clear();
    components_.clear();
}

// add a child entity
void Entity::add_child(std::shared_ptr<Entity> child)
{
    if (child->parent_)
    {
        // if the child has a parent, remove it from the original parent
        auto& siblings = child->parent_->children_;
        auto it = std::find(siblings.begin(), siblings.end(), child);
        if (it != siblings.end())
        {
            siblings.erase(it);
        }
    }
    
    child->parent_ = this;
    children_.push_back(child);
}

} // namespace lz 