#include "Component.h"
#include "Entity.h"

namespace lz
{

Component::Component(Entity* entity)
    : entity_(entity)
{
}

} // namespace lz 