#pragma once

#include <iostream>

// 定义入口点宏，用于快速生成应用程序入口点
// 使用方法：LINGZE_MAIN(AppClassName)
// 其中AppClassName是继承自lz::App的应用程序类名

#define LINGZE_MAIN(AppClass) int main() { \
    try { \
        AppClass app; \
        return app.run(); \
    } catch (const std::exception& e) { \
        std::cerr << "程序异常: " << e.what() << std::endl; \
        return -1; \
    } catch (...) { \
        std::cerr << "程序发生未知异常！" << std::endl; \
        return -1; \
    } \
}