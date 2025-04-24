#include "ImGuiProfilerRenderer.h"

#include <array>
#include "algorithm"

namespace ImGuiUtils
{
	glm::vec2 vec2(ImVec2 vec)
	{
		return glm::vec2(vec.x, vec.y);
	}

	ProfilerGraph::ProfilerGraph(const size_t frames_count)
	{
		frames_.resize(frames_count);
		for (auto& frame : frames_)
			frame.tasks.reserve(100);
		frame_width = 3;
		frame_spacing = 1;
		use_colored_legend_text = false;
	}

	void ProfilerGraph::load_frame_data(const lz::ProfilerTask* tasks, const size_t count)
	{
		auto& curr_frame = frames_[curr_frame_index_];
		curr_frame.tasks.resize(0);
		for (size_t task_index = 0; task_index < count; task_index++)
		{
			if (task_index == 0)
				curr_frame.tasks.push_back(tasks[task_index]);
			else
			{
				if (tasks[task_index - 1].color != tasks[task_index].color || tasks[task_index - 1].name != tasks[
					task_index].name)
				{
					curr_frame.tasks.push_back(tasks[task_index]);
				}
				else
				{
					curr_frame.tasks.back().end_time = tasks[task_index].end_time;
				}
			}
		}
		curr_frame.task_stats_index.resize(curr_frame.tasks.size());

		for (size_t task_index = 0; task_index < curr_frame.tasks.size(); task_index++)
		{
			auto& task = curr_frame.tasks[task_index];
			auto it = task_name_to_stats_index_.find(task.name);
			if (it == task_name_to_stats_index_.end())
			{
				task_name_to_stats_index_[task.name] = task_stats_.size();
				TaskStats taskStat;
				taskStat.max_time = -1.0;
				task_stats_.push_back(taskStat);
			}
			curr_frame.task_stats_index[task_index] = task_name_to_stats_index_[task.name];
		}
		curr_frame_index_ = (curr_frame_index_ + 1) % frames_.size();

		rebuild_task_stats(curr_frame_index_, 300/*frames_.size()*/);
	}

	void ProfilerGraph::render_timings(const int graph_width, const int legend_width, const int height, const int frame_index_offset)
	{
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		const glm::vec2 widget_pos = vec2(ImGui::GetCursorScreenPos());
		render_graph(draw_list, widget_pos, glm::vec2(graph_width, height), frame_index_offset);
		render_legend(draw_list, widget_pos + glm::vec2(graph_width, 0.0f), glm::vec2(legend_width, height),
		             frame_index_offset);
		ImGui::Dummy(ImVec2(float(graph_width + legend_width), float(height)));
	}

	void ProfilerGraph::rebuild_task_stats(const size_t end_frame, const size_t frames_count)
	{
		for (auto& task_stat : task_stats_)
		{
			task_stat.max_time = -1.0f;
			task_stat.priority_order = size_t(-1);
			task_stat.on_screen_index = size_t(-1);
		}

		for (size_t frame_number = 0; frame_number < frames_count; frame_number++)
		{
			const size_t frameIndex = (end_frame - 1 - frame_number + frames_.size()) % frames_.size();
			auto& frame = frames_[frameIndex];
			for (size_t task_index = 0; task_index < frame.tasks.size(); task_index++)
			{
				const auto& task = frame.tasks[task_index];
				auto& stats = task_stats_[frame.task_stats_index[task_index]];
				stats.max_time = std::max(stats.max_time, task.end_time - task.start_time);
			}
		}
		std::vector<size_t> stat_priorities;
		stat_priorities.resize(task_stats_.size());
		for (size_t stat_index = 0; stat_index < task_stats_.size(); stat_index++)
			stat_priorities[stat_index] = stat_index;

		std::sort(stat_priorities.begin(), stat_priorities.end(), [this](const size_t left, const size_t right)
		{
			return task_stats_[left].max_time > task_stats_[right].max_time;
		});
		for (size_t stat_number = 0; stat_number < task_stats_.size(); stat_number++)
		{
			const size_t stat_index = stat_priorities[stat_number];
			task_stats_[stat_index].priority_order = stat_number;
		}
	}

	void ProfilerGraph::render_graph(ImDrawList* draw_list,  glm::vec2 graph_pos,  glm::vec2 graph_size,
	                                 size_t frame_index_offset) const
	{
		rect(draw_list, graph_pos, graph_pos + graph_size, 0xffffffff, false);
		constexpr float max_frame_time = 1.0f / 30.0f;

		for (size_t frame_number = 0; frame_number < frames_.size(); frame_number++)
		{
			const size_t frame_index = (curr_frame_index_ - frame_index_offset - 1 - frame_number + 2 * frames_.size()) % frames_.
				size();

			glm::vec2 frame_pos = graph_pos + glm::vec2(
				graph_size.x - 1 - frame_width - (frame_width + frame_spacing) * frame_number, graph_size.y - 1);
			if (frame_pos.x < graph_pos.x + 1)
				break;
			glm::vec2 task_pos = frame_pos + glm::vec2(0.0f, 0.0f);
			auto& frame = frames_[frame_index];
			for (const auto task : frame.tasks)
			{
				constexpr float height_threshold = 1.0f;
				const float task_start_height = (float(task.start_time) / max_frame_time) * graph_size.y;
				const float task_end_height = (float(task.end_time) / max_frame_time) * graph_size.y;
				//taskMaxCosts[task.name] = std::max(taskMaxCosts[task.name], task.endTime - task.startTime);
				if (abs(task_end_height - task_start_height) > height_threshold)
					rect(draw_list, task_pos + glm::vec2(0.0f, -task_start_height),
					     task_pos + glm::vec2(frame_width, -task_end_height), task.color, true);
			}
		}
	}

	void ProfilerGraph::render_legend(ImDrawList* draw_list, glm::vec2 legend_pos, glm::vec2 legend_size,
	                                 size_t frame_index_offset)
	{
		float marker_left_rect_margin = 3.0f;
		float marker_left_rect_width = 5.0f;
		float max_frame_time = 1.0f / 30.0f;
		float marker_mid_width = 30.0f;
		float marker_right_rect_width = 10.0f;
		float marker_rigth_rect_margin = 3.0f;
		float marker_right_rect_height = 10.0f;
		float marker_right_rect_spacing = 4.0f;
		float name_offset = 30.0f;
		glm::vec2 text_margin = glm::vec2(5.0f, -3.0f);

		auto& curr_frame = frames_[(curr_frame_index_ - frame_index_offset - 1 + 2 * frames_.size()) % frames_.size()];
		size_t max_tasks_count = size_t(legend_size.y / (marker_right_rect_height + marker_right_rect_spacing));

		for (auto& task_stat : task_stats_)
		{
			task_stat.on_screen_index = size_t(-1);
		}

		size_t tasks_to_show = std::min<size_t>(task_stats_.size(), max_tasks_count);
		size_t tasks_shown_count = 0;
		for (size_t task_index = 0; task_index < curr_frame.tasks.size(); task_index++)
		{
			auto& task = curr_frame.tasks[task_index];
			auto& stat = task_stats_[curr_frame.task_stats_index[task_index]];

			if (stat.priority_order >= tasks_to_show)
				continue;

			if (stat.on_screen_index == size_t(-1))
			{
				stat.on_screen_index = tasks_shown_count++;
			}
			else
				continue;
			float task_start_height = (float(task.start_time) / max_frame_time) * legend_size.y;
			float task_end_height = (float(task.end_time) / max_frame_time) * legend_size.y;

			glm::vec2 marker_left_rect_min = legend_pos + glm::vec2(marker_left_rect_margin, legend_size.y);
			glm::vec2 marker_left_rect_max = marker_left_rect_min + glm::vec2(marker_left_rect_width, 0.0f);
			marker_left_rect_min.y -= task_start_height;
			marker_left_rect_max.y -= task_end_height;

			glm::vec2 marker_right_rect_min = legend_pos + glm::vec2(
				marker_left_rect_margin + marker_left_rect_width + marker_mid_width,
				legend_size.y - marker_rigth_rect_margin - (marker_right_rect_height + marker_right_rect_spacing) * stat.
				on_screen_index);
			glm::vec2 marker_right_rect_max = marker_right_rect_min + glm::vec2(
				marker_right_rect_width, -marker_right_rect_height);
			render_task_marker(draw_list, marker_left_rect_min, marker_left_rect_max, marker_right_rect_min, marker_right_rect_max,
			                 task.color);

			uint32_t text_color = use_colored_legend_text ? task.color : lz::Colors::imgui_text; // task.color;

			float task_time_ms = float(task.end_time - task.start_time);
			std::ostringstream time_text;
			time_text.precision(2);
			time_text << std::fixed << std::string("[") << (task_time_ms * 1000.0f);

			text(draw_list, marker_right_rect_max + text_margin, text_color, time_text.str().c_str());
			text(draw_list, marker_right_rect_max + text_margin + glm::vec2(name_offset, 0.0f), text_color,
			     (std::string("ms] ") + task.name).c_str());
		}

		/*
			struct PriorityEntry
			{
			  bool isUsed;
			  lz::ProfilerTask task;
			};
			std::map<std::string, PriorityEntry> priorityEntries;
			for (auto priorityTask : priorityTasks)
			{
			  PriorityEntry entry;
			  entry.task = frames_[priorityTask.frameIndex].tasks[priorityTask.taskIndex];
			  entry.isUsed = false;
			  priorityEntries[entry.task.name] = entry;
			}
			size_t shownTasksCount = 0;
			for (size_t taskIndex = 0; taskIndex < currFrame.tasks.size(); taskIndex++)
			{
			  auto &task = currFrame.tasks[taskIndex];
			  auto it = priorityEntries.find(task.name);
			  if (it != priorityEntries.end() && !it->second.isUsed)
			  {
			    it->second.isUsed = true;

			    float taskStartHeight = (float(task.startTime) / maxFrameTime) * legendSize.y;
			    float taskEndHeight = (float(task.endTime) / maxFrameTime) * legendSize.y;


			    glm::vec2 markerLeftRectMin = legendPos + glm::vec2(markerLeftRectMargin, legendSize.y);
			    glm::vec2 markerLeftRectMax = markerLeftRectMin + glm::vec2(markerLeftRectWidth, 0.0f);
			    markerLeftRectMin.y -= taskStartHeight;
			    markerLeftRectMax.y -= taskEndHeight;

			    glm::vec2 markerRightRectMin = legendPos + glm::vec2(markerLeftRectMargin + markerLeftRectWidth + markerMidWidth, legendSize.y - markerRigthRectMargin - (markerRightRectHeight + markerRightRectSpacing) * shownTasksCount);
			    glm::vec2 markerRightRectMax = markerRightRectMin + glm::vec2(markerRightRectWidth, -markerRightRectHeight);
			    RenderTaskMarker(drawList, markerLeftRectMin, markerLeftRectMax, markerRightRectMin, markerRightRectMax, task.color);

			    uint32_t textColor = lz::Colors::imguiText;// task.color;

			    float taskTimeMs = float(task.endTime - task.startTime);
			    std::ostringstream timeText;
			    timeText.precision(2);
			    timeText << std::fixed << std::string("[") << (taskTimeMs * 1000.0f);

			    Text(drawList, markerRightRectMax + textMargin, textColor, timeText.str().c_str());
			    Text(drawList, markerRightRectMax + textMargin + glm::vec2(nameOffset, 0.0f), textColor, (std::string("ms] ") + task.name).c_str());
			    shownTasksCount++;
			  }
			}*/

		/*for (size_t priorityTaskIndex = 0; priorityTaskIndex < priorityTasks.size(); priorityTaskIndex++)
			{
			  auto &priorityTask = priorityTasks[priorityTaskIndex];
			  auto &globalTask = frames_[priorityTask.frameIndex].tasks[priorityTask.taskIndex];

			  size_t lastFrameTaskIndex = currFrame.FindTask(globalTask.name);

			  glm::vec2 taskPos = legendPos + marginSpacing + glm::vec2(0.0f, markerHeight) + glm::vec2(0.0f, (markerHeight + itemSpacing) * priorityTaskIndex);
			  Rect(drawList, taskPos, taskPos + glm::vec2(markerHeight, -markerHeight), task.color, true);
			  Text(drawList, taskPos + textOffset, 0xffffffff, task.name.c_str());
			}*/
	}

	void ProfilerGraph::rect(ImDrawList* draw_list,  glm::vec2 min_point,  glm::vec2 max_point,  uint32_t col,  bool filled)
	{
		if (filled)
			draw_list->AddRectFilled(ImVec2(min_point.x, min_point.y), ImVec2(max_point.x, max_point.y), col);
		else
			draw_list->AddRect(ImVec2(min_point.x, min_point.y), ImVec2(max_point.x, max_point.y), col);
	}

	void ProfilerGraph::text(ImDrawList* draw_list, glm::vec2 point, uint32_t col,const  char* text)
	{
		draw_list->AddText(ImVec2(point.x, point.y), col, text);
	}

	void ProfilerGraph::triangle(ImDrawList* draw_list, const std::array<glm::vec2, 3>& points,  uint32_t col,  bool filled)
	{
		if (filled)
			draw_list->AddTriangleFilled(ImVec2(points[0].x, points[0].y), ImVec2(points[1].x, points[1].y),
			                            ImVec2(points[2].x, points[2].y), col);
		else
			draw_list->AddTriangle(ImVec2(points[0].x, points[0].y), ImVec2(points[1].x, points[1].y),
			                      ImVec2(points[2].x, points[2].y), col);
	}

	void ProfilerGraph::render_task_marker(ImDrawList* draw_list, glm::vec2 left_min_point, glm::vec2 left_max_point,
	                                     glm::vec2 right_min_point, glm::vec2 right_max_point, uint32_t col)
	{
		rect(draw_list, left_min_point, left_max_point, col, true);
		rect(draw_list, right_min_point, right_max_point, col, true);
		std::array<ImVec2, 4> points = {
			ImVec2(left_max_point.x, left_min_point.y),
			ImVec2(left_max_point.x, left_max_point.y),
			ImVec2(right_min_point.x, right_max_point.y),
			ImVec2(right_min_point.x, right_min_point.y)
		};
		draw_list->AddConvexPolyFilled(points.data(), int(points.size()), col);
	}

	ProfilersWindow::ProfilersWindow():
		cpu_graph(300),
		gpu_graph(300)
	{
		stop_profiling = false;
		frame_offset = 0;
		frame_width = 3;
		frame_spacing = 1;
		use_colored_legend_text = true;
		prev_fps_frame_time = std::chrono::system_clock::now();
		fps_frames_count = 0;
		avg_frame_time = 1.0f;
	}

	void ProfilersWindow::render()
	{
		fps_frames_count++;
		const auto curr_frame_time = std::chrono::system_clock::now();
		{
			float fps_delta_time = std::chrono::duration<float>(curr_frame_time - prev_fps_frame_time).count();
			if (fps_delta_time > 0.5f)
			{
				this->avg_frame_time = fps_delta_time / float(fps_frames_count);
				fps_frames_count = 0;
				prev_fps_frame_time = curr_frame_time;
			}
		}

		std::stringstream title;
		title.precision(2);
		title << std::fixed << "Legit profiler [" << 1.0f / avg_frame_time << "fps\t" << avg_frame_time * 1000.0f <<
			"ms]###ProfilerWindow";
		//###AnimatedTitle
		ImGui::Begin(title.str().c_str(), 0, ImGuiWindowFlags_NoScrollbar);
		ImVec2 canvas_size = ImGui::GetContentRegionAvail();

		int size_margin = int(ImGui::GetStyle().ItemSpacing.y);
		int max_graph_height = 300;
		int available_graph_height = (int(canvas_size.y) - size_margin) / 2;
		int graph_height = std::min(max_graph_height, available_graph_height);
		int legend_width = 200;
		int graph_width = int(canvas_size.x) - legend_width;
		gpu_graph.render_timings(graph_width, legend_width, graph_height, frame_offset);
		cpu_graph.render_timings(graph_width, legend_width, graph_height, frame_offset);
		if (graph_height * 2 + size_margin + size_margin < canvas_size.y)
		{
			ImGui::Columns(2);
			size_t textSize = 50;
			ImGui::Checkbox("Stop profiling", &stop_profiling);
			//ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - textSize);
			ImGui::Checkbox("Colored legend text", &use_colored_legend_text);
			ImGui::DragInt("Frame offset", &frame_offset, 1.0f, 0, 400);
			ImGui::NextColumn();

			ImGui::SliderInt("Frame width", &frame_width, 1, 4);
			ImGui::SliderInt("Frame spacing", &frame_spacing, 0, 2);
			ImGui::SliderFloat("Transparency", &ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w, 0.0f, 1.0f);
			ImGui::Columns(1);
		}
		if (!stop_profiling)
			frame_offset = 0;
		gpu_graph.frame_width = frame_width;
		gpu_graph.frame_spacing = frame_spacing;
		gpu_graph.use_colored_legend_text = use_colored_legend_text;
		cpu_graph.frame_width = frame_width;
		cpu_graph.frame_spacing = frame_spacing;
		cpu_graph.use_colored_legend_text = use_colored_legend_text;

		ImGui::End();
	}
}
