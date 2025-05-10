#include "Transform.h"
#include "Entity.h"

namespace lz
{

Transform::Transform(Entity* entity)
    : Component(entity), 
      position_(0.0f), 
      rotation_(0.0f), 
      scale_(1.0f, 1.0f, 1.0f),
      matrix_dirty_(true)
{
}

void Transform::set_position(const glm::vec3& position)
{
    position_ = position;
    matrix_dirty_ = true;
}

void Transform::set_rotation(const glm::vec3& rotation)
{
    rotation_ = rotation;
    matrix_dirty_ = true;
}

void Transform::set_scale(const glm::vec3& scale)
{
    scale_ = scale;
    matrix_dirty_ = true;
}

glm::mat4 Transform::get_local_matrix()
{
    if (matrix_dirty_)
    {
        // calculate the local matrix TRS
        glm::mat4 translate_matrix = glm::translate(glm::mat4(1.0f), position_);
        
        // the rotation (note: using radian)
        glm::mat4 rotate_x = glm::rotate(glm::mat4(1.0f), rotation_.x, glm::vec3(1, 0, 0));
        glm::mat4 rotate_y = glm::rotate(glm::mat4(1.0f), rotation_.y, glm::vec3(0, 1, 0));
        glm::mat4 rotate_z = glm::rotate(glm::mat4(1.0f), rotation_.z, glm::vec3(0, 0, 1));
        glm::mat4 rotate_matrix = rotate_z * rotate_y * rotate_x;
        
        glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), scale_);
        
        local_matrix_ = translate_matrix * rotate_matrix * scale_matrix;
        matrix_dirty_ = false;
    }
    
    return local_matrix_;
}

glm::mat4 Transform::get_world_matrix()
{
    glm::mat4 local = get_local_matrix();
    
    // if there is a parent entity, multiply with the parent entity's world matrix
    if (entity_->get_parent())
    {
        Transform* parent_transform = entity_->get_parent()->get_transform();
        return parent_transform->get_world_matrix() * local;
    }
    
    return local;
}

} // namespace lz 