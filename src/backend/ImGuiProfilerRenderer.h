#pragma once

#include <chrono>
#include <ios>
#include <map>
#include <sstream>
#include <vector>

#include "ProfilerTask.h"
#include "imgui.h"

#include "Config.h"
#include "glm/glm.hpp"

namespace ImGuiUtils
{
glm::vec2 vec2(ImVec2 vec);

class ProfilerGraph
{
  public:
	int  frame_width;
	int  frame_spacing;
	bool use_colored_legend_text;

	ProfilerGraph(size_t frames_count);

	void load_frame_data(const lz::ProfilerTask *tasks, size_t count);

	void render_timings(int graph_width, int legend_width, int height, int frame_index_offset);

  private:
	void rebuild_task_stats(size_t end_frame, size_t frames_count);

	void render_graph(ImDrawList *draw_list, glm::vec2 graph_pos, glm::vec2 graph_size,
	                  size_t frame_index_offset) const;

	void render_legend(ImDrawList *draw_list, glm::vec2 legend_pos, glm::vec2 legend_size,
	                   size_t frame_index_offset);

	static void rect(ImDrawList *draw_list, glm::vec2 min_point, glm::vec2 max_point, uint32_t col,
	                 bool filled = true);

	static void text(ImDrawList *draw_list, glm::vec2 point, uint32_t col, const char *text);

	static void triangle(ImDrawList *draw_list, const std::array<glm::vec2, 3> &points, uint32_t col,
	                     bool filled = true);

	static void render_task_marker(ImDrawList *draw_list, glm::vec2 left_min_point, glm::vec2 left_max_point,
	                               glm::vec2 right_min_point, glm::vec2 right_max_point, uint32_t col);

	struct FrameData
	{
		std::vector<lz::ProfilerTask> tasks;
		std::vector<size_t>           task_stats_index;
	};

	struct TaskStats
	{
		double max_time;
		size_t priority_order;
		size_t on_screen_index;
	};

	std::vector<TaskStats>        task_stats_;
	std::map<std::string, size_t> task_name_to_stats_index_;

	std::vector<FrameData> frames_;
	size_t                 curr_frame_index_ = 0;
};

class ProfilersWindow
{
  public:
	ProfilersWindow();

	void render();

	bool          stop_profiling;
	int           frame_offset;
	ProfilerGraph cpu_graph;
	ProfilerGraph gpu_graph;
	int           frame_width;
	int           frame_spacing;
	bool          use_colored_legend_text;
	using time_point = std::chrono::time_point<std::chrono::system_clock>;
	time_point prev_fps_frame_time;
	size_t     fps_frames_count;
	float      avg_frame_time;
};
}        // namespace ImGuiUtils
