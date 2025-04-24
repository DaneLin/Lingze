#pragma once

#include "backend/App.h"

namespace lz {
namespace application {

// Define DemoApp class, inherits from App base class
class DemoApp : public lz::App
{
public:
    DemoApp() : lz::App("Lingze Demo", 1280, 760) {}
    
    virtual ~DemoApp() = default;
    
protected:
    // Load scene
    virtual bool load_scene() override
    {
        // Demo application can use different scenes
        std::string config_file_name = std::string(SCENE_DIR) + "CubeScene.json";  // Can also use other scenes
        lz::Scene::GeometryTypes geo_type = lz::Scene::GeometryTypes::eTriangles;
        
        return load_scene_from_file(config_file_name, geo_type);
    }
    
    // Process input
    virtual void process_input() override
    {
        // Call base class handling
        lz::App::process_input();
        
        // Special input processing can be added in demo application
    }
    
    // Update logic
    virtual void update(float deltaTime) override
    {
        // Special update logic can be added in demo application
    }
};

} // namespace application
} // namespace lz 