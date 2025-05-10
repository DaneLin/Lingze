#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <typeinfo>
#include <typeindex>

// forward declaration of the component class
namespace lz
{
class Component;
class Transform;

/**
 * @brief Entity is the basic object in the scene.
 */
class Entity
{
  public:
    Entity(const std::string& name = "Entity");
    ~Entity();

    // get the entity name
    const std::string& get_name() const { return name_; }
    
    // set the entity name
    void set_name(const std::string& name) { name_ = name; }
    
    // get the entity id
    uint64_t get_id() const { return id_; }
    
    // add a component
    template<typename T, typename... Args>
    T* add_component(Args&&... args)
    {
        // get the component type id
        std::type_index type_id = std::type_index(typeid(T));
        
        // check if the component exists
        auto it = components_.find(type_id.hash_code());
        if (it != components_.end())
        {
            return static_cast<T*>(it->second.get());
        }
        
        // create a new component
        auto component = std::make_unique<T>(this, std::forward<Args>(args)...);
        T* component_ptr = component.get();
        
        // add to the component list
        components_[type_id.hash_code()] = std::move(component);
        
        return component_ptr;
    }
    
    // get a component
    template<typename T>
    T* get_component()
    {
        std::type_index type_id = std::type_index(typeid(T));
        auto it = components_.find(type_id.hash_code());
        if (it != components_.end())
        {
            return static_cast<T*>(it->second.get());
        }
        return nullptr;
    }
    
    // check if has a component
    template<typename T>
    bool has_component()
    {
        std::type_index type_id = std::type_index(typeid(T));
        return components_.find(type_id.hash_code()) != components_.end();
    }
    
    // get the transform component
    Transform* get_transform() { return transform_; }
    
    // add a child entity
    void add_child(std::shared_ptr<Entity> child);
    
    // get the parent entity
    Entity* get_parent() { return parent_; }
    
    // get the children entities
    const std::vector<std::shared_ptr<Entity>>& get_children() const { return children_; }

  private:
    uint64_t id_;                                // the unique id of the entity 
    std::string name_;                           // the name of the entity
    Entity* parent_;                             // the parent entity
    std::vector<std::shared_ptr<Entity>> children_; // the children entities
    Transform* transform_;                       // the transform component (each entity must have)
    std::unordered_map<size_t, std::unique_ptr<Component>> components_; // the component list
    
    static uint64_t next_id_;                    // the next entity id
};

} // namespace lz 