#pragma once
#include "Handles.h"
#include <assert.h>
#include <chrono>
#include <vector>

#include "ProfilerTask.h"
#include <string>

namespace lz
{
class CpuProfiler
{
  public:
	CpuProfiler();

	size_t start_task(const std::string &task_name, uint32_t task_color);

	ProfilerTask end_task(size_t task_id);

	size_t start_frame();

	void end_frame(size_t frame_id);

	const std::vector<ProfilerTask> &get_profiler_tasks();

  private:
	double get_curr_frame_time_seconds() const;

	struct TaskHandleInfo
	{
		TaskHandleInfo(CpuProfiler *profiler, size_t task_id);

		void         reset() const;
		CpuProfiler *profiler;
		size_t       task_id;
	};
	struct FrameHandleInfo
	{
		FrameHandleInfo(CpuProfiler *profiler, size_t frame_id);

		void         reset() const;
		CpuProfiler *profiler;
		size_t       frame_id;
	};

  public:
	using ScopedTask = UniqueHandle<TaskHandleInfo, CpuProfiler>;
	ScopedTask start_scoped_task(const std::string &task_name, uint32_t task_color);
	using ScopedFrame = UniqueHandle<FrameHandleInfo, CpuProfiler>;
	ScopedFrame start_scoped_frame();

  private:
	using hrc = std::chrono::high_resolution_clock;
	size_t                        frame_index_;
	std::vector<lz::ProfilerTask> profiler_tasks_;
	hrc::time_point               frame_start_time_;
	friend struct UniqueHandle<TaskHandleInfo, CpuProfiler>;
};
}        // namespace lz
