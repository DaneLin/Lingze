#pragma once

#include "backend/App.h"

namespace lz {
namespace application {

// 定义DemoApp类，继承自App基类
class DemoApp : public lz::App
{
public:
    DemoApp() : lz::App("Lingze Demo", 1280, 760) {}
    
    virtual ~DemoApp() = default;
    
protected:
    // 加载场景
    virtual bool load_scene() override
    {
        // 演示应用可以使用不同的场景
        std::string config_file_name = std::string(SCENE_DIR) + "CubeScene.json";  // 也可以换成其他场景
        lz::Scene::GeometryTypes geo_type = lz::Scene::GeometryTypes::eTriangles;
        
        return load_scene_from_file(config_file_name, geo_type);
    }
    
    // 处理输入
    virtual void process_input() override
    {
        // 调用基类处理
        lz::App::process_input();
        
        // 演示应用中可添加特殊的输入处理
    }
    
    // 更新逻辑
    virtual void update(float deltaTime) override
    {
        // 演示应用中可添加特殊的更新逻辑
    }
};

} // namespace application
} // namespace lz 