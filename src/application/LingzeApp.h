#pragma once

#include "backend/App.h"
#include "render/renderers/SimpleRenderer.h"

namespace lz {
namespace application {

// 定义LingzeApp类，继承自App基类
class LingzeApp : public lz::App
{
public:
    LingzeApp() : lz::App("Lingze", 1280, 760) {}
    
    virtual ~LingzeApp() = default;
    
protected:
    // 加载场景
    virtual bool load_scene() override
    {
        // 使用立方体场景
        std::string config_file_name = std::string(SCENE_DIR) + "CubeScene.json";
        lz::Scene::GeometryTypes geo_type = lz::Scene::GeometryTypes::eTriangles;
        
        return load_scene_from_file(config_file_name, geo_type);
    }
    
    // 创建渲染器
    virtual std::unique_ptr<lz::render::BaseRenderer> create_renderer() override
    {
        // 使用简单渲染器
        return std::make_unique<lz::render::SimpleRenderer>(core_.get());
    }
};

} // namespace application
} // namespace lz 