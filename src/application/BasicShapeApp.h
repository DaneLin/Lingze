#pragma once

#include "backend/App.h"

namespace lz {
    namespace application {

        // Define DemoApp class, inherits from App base class
        class BasicShapeApp : public lz::App
        {
        public:
            BasicShapeApp() : lz::App("Lingze Basic Shader", 1280, 760) {}

            virtual ~BasicShapeApp() = default;

        protected:
            // Load scene
            virtual bool load_scene() override;

            // Create renderer
            virtual std::unique_ptr<lz::render::BaseRenderer> create_renderer() override;
        };

    } // namespace application
} // namespace lz 