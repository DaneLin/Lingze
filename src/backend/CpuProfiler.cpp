#include "CpuProfiler.h"

namespace lz
{
CpuProfiler::CpuProfiler()
{
	frame_index_ = 0;
}

size_t CpuProfiler::start_task(const std::string &task_name, const uint32_t task_color)
{
	lz::ProfilerTask task;
	task.color           = task_color;
	task.name            = task_name;
	task.start_time      = get_curr_frame_time_seconds();
	task.end_time        = -1.0;
	const size_t task_id = profiler_tasks_.size();
	profiler_tasks_.push_back(task);
	return task_id;
}

ProfilerTask CpuProfiler::end_task(const size_t task_id)
{
	assert(profiler_tasks_.size() == task_id + 1 && profiler_tasks_.back().end_time < 0.0);
	profiler_tasks_.back().end_time = get_curr_frame_time_seconds();
	return profiler_tasks_.back();
}

size_t CpuProfiler::start_frame()
{
	profiler_tasks_.clear();
	frame_start_time_ = hrc::now();
	return frame_index_;
}

void CpuProfiler::end_frame(const size_t frame_id)
{
	assert(frame_id == frame_index_);
	frame_index_++;
}

const std::vector<ProfilerTask> &CpuProfiler::get_profiler_tasks()
{
	return profiler_tasks_;
}

double CpuProfiler::get_curr_frame_time_seconds() const
{
	return double(std::chrono::duration_cast<std::chrono::microseconds>(hrc::now() - frame_start_time_).count()) / 1e6;
}

CpuProfiler::TaskHandleInfo::TaskHandleInfo(CpuProfiler *profiler, const size_t task_id)
{
	this->profiler = profiler;
	this->task_id  = task_id;
}

void CpuProfiler::TaskHandleInfo::reset() const
{
	profiler->end_task(task_id);
}

CpuProfiler::FrameHandleInfo::FrameHandleInfo(CpuProfiler *profiler, const size_t frame_id)
{
	this->profiler = profiler;
	this->frame_id = frame_id;
}

void CpuProfiler::FrameHandleInfo::reset() const
{
	profiler->end_frame(frame_id);
}

CpuProfiler::ScopedTask CpuProfiler::start_scoped_task(const std::string &task_name, uint32_t task_color)
{
	return ScopedTask(TaskHandleInfo(this, start_task(task_name, task_color)), true);
}

CpuProfiler::ScopedFrame CpuProfiler::start_scoped_frame()
{
	return ScopedFrame(FrameHandleInfo(this, start_frame()), true);
}
}        // namespace lz
