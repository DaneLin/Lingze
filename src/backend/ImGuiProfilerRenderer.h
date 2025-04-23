#pragma once

#include <chrono>
#include <ios>
#include <map>
#include <sstream>
#include <vector>

#include "imgui.h"
#include "ProfilerTask.h"

#include "LingzeVK.h"

namespace ImGuiUtils
{
	glm::vec2 Vec2(ImVec2 vec);

	class ProfilerGraph
	{
	public:
		int frameWidth;
		int frameSpacing;
		bool useColoredLegendText;

		ProfilerGraph(size_t framesCount);

		void LoadFrameData(const lz::ProfilerTask* tasks, size_t count);

		void RenderTimings(int graphWidth, int legendWidth, int height, int frameIndexOffset);

	private:
		void RebuildTaskStats(size_t endFrame, size_t framesCount);

		void RenderGraph(ImDrawList* drawList, glm::vec2 graphPos, glm::vec2 graphSize, size_t frameIndexOffset);

		void RenderLegend(ImDrawList* drawList, glm::vec2 legendPos, glm::vec2 legendSize, size_t frameIndexOffset);

		static void Rect(ImDrawList* drawList, glm::vec2 minPoint, glm::vec2 maxPoint, uint32_t col,
		                 bool filled = true);

		static void Text(ImDrawList* drawList, glm::vec2 point, uint32_t col, const char* text);

		static void Triangle(ImDrawList* drawList, std::array<glm::vec2, 3> points, uint32_t col, bool filled = true);

		static void RenderTaskMarker(ImDrawList* drawList, glm::vec2 leftMinPoint, glm::vec2 leftMaxPoint,
		                             glm::vec2 rightMinPoint, glm::vec2 rightMaxPoint, uint32_t col);

		struct FrameData
		{
			std::vector<lz::ProfilerTask> tasks;
			std::vector<size_t> taskStatsIndex;
		};

		struct TaskStats
		{
			double maxTime;
			size_t priorityOrder;
			size_t onScreenIndex;
		};

		std::vector<TaskStats> taskStats;
		std::map<std::string, size_t> taskNameToStatsIndex;


		std::vector<FrameData> frames;
		size_t currFrameIndex = 0;
	};

	class ProfilersWindow
	{
	public:
		ProfilersWindow();

		void Render();

		bool stopProfiling;
		int frameOffset;
		ProfilerGraph cpuGraph;
		ProfilerGraph gpuGraph;
		int frameWidth;
		int frameSpacing;
		bool useColoredLegendText;
		using TimePoint = std::chrono::time_point<std::chrono::system_clock>;
		TimePoint prevFpsFrameTime;
		size_t fpsFramesCount;
		float avgFrameTime;
	};
}
