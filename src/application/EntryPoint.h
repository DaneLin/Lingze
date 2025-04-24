#pragma once

#include <iostream>

// Define entry point macro for quick application entry point generation
// Usage: LINGZE_MAIN(AppClassName)
// Where AppClassName is the name of a class that inherits from lz::App

#define LINGZE_MAIN(AppClass) int main() { \
    try { \
        AppClass app; \
        return app.run(); \
    } catch (const std::exception& e) { \
        std::cerr << "Program exception: " << e.what() << std::endl; \
        return -1; \
    } catch (...) { \
        std::cerr << "Unknown program exception occurred!" << std::endl; \
        return -1; \
    } \
}