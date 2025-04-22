// Lingze.cpp: 定义应用程序的入口点。
//
#include "backend/LingzeVK.h"

#define GLM_FORCE_RADIANS
#define GLM_DEPTH_ZERO_TO_ONE
//#define GLM_FORCE_RIGHT_HANDED
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/transform.hpp>
#include <tiny_obj_loader.h>

#include <iostream>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "json/json.h"
#include <chrono>
#include <ctime>
#include <memory>
#include <sstream>
#include <array>

glm::vec3 ReadJsonVec3f(Json::Value vectorValue)
{
	return glm::vec3(vectorValue[0].asFloat(), vectorValue[1].asFloat(), vectorValue[2].asFloat());
}
glm::ivec2 ReadJsonVec2i(Json::Value vectorValue)
{
	return glm::ivec2(vectorValue[0].asInt(), vectorValue[1].asInt());
}

glm::uvec3 ReadJsonVec3u(Json::Value vectorValue)
{
	return glm::uvec3(vectorValue[0].asUInt(), vectorValue[1].asUInt(), vectorValue[2].asUInt());
}

#include "imgui.h"
#include "backend/ImGuiProfilerRenderer.h"

using namespace std;

int main()
{
	cout << "Hello CMake." << endl;
	return 0;
}
