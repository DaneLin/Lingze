#pragma once

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#define LOGGER_FORMAT "[%^%l%$] %v"
#define PROJECT_NAME "Lingze"

// Mainly for IDEs
#ifndef ROOT_PATH_SIZE
#	define ROOT_PATH_SIZE 0
#endif

#define __FILENAME__ (static_cast<const char *>(__FILE__) + ROOT_PATH_SIZE)

#define LOGI(...) spdlog::info(fmt::format(__VA_ARGS__))
#define LOGW(...) spdlog::warn(fmt::format(__VA_ARGS__))
#define LOGE(...) spdlog::error("[{}:{}] {}", __FILENAME__, __LINE__, fmt::format(__VA_ARGS__))
#define LOGD(...) spdlog::debug(fmt::format(__VA_ARGS__))

#ifdef _DEBUG
#	define DLOGI(...) spdlog::info(fmt::format(__VA_ARGS__))
#	define DLOGW(...) spdlog::warn(fmt::format(__VA_ARGS__))
#	define DLOGE(...) spdlog::error("[{}:{}] {}", __FILENAME__, __LINE__, fmt::format(__VA_ARGS__))
#	define DLOGD(...) spdlog::debug(fmt::format(__VA_ARGS__))
#else
#	define DLOGI(...)
#	define DLOGW(...)
#	define DLOGE(...)
#	define DLOGD(...)
#endif
