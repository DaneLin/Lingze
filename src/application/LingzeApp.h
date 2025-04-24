#pragma once

#include "backend/App.h"
#include "render/renderers/SimpleRenderer.h"

namespace lz {
namespace application {

// Define LingzeApp class, inherits from App base class
class LingzeApp : public lz::App
{
public:
    LingzeApp() : lz::App("Lingze", 1280, 760) {}
    
    virtual ~LingzeApp() = default;
    
protected:
    // Load scene
    virtual bool load_scene() override
    {
        // Use cube scene
        std::string config_file_name = std::string(SCENE_DIR) + "CubeScene.json";
        lz::Scene::GeometryTypes geo_type = lz::Scene::GeometryTypes::eTriangles;
        
        return load_scene_from_file(config_file_name, geo_type);
    }
    
    // Create renderer
    virtual std::unique_ptr<lz::render::BaseRenderer> create_renderer() override
    {
        // Use simple renderer
        return std::make_unique<lz::render::SimpleRenderer>(core_.get());
    }
};

} // namespace application
} // namespace lz 