#include "CpuProfiler.h"

namespace lz
{
	CpuProfiler::CpuProfiler()
	{
		frameIndex = 0;
	}

	size_t CpuProfiler::StartTask(std::string taskName, uint32_t taskColor)
	{
		lz::ProfilerTask task;
		task.color = taskColor;
		task.name = taskName;
		task.startTime = GetCurrFrameTimeSeconds();
		task.endTime = -1.0;
		size_t taskId = profilerTasks.size();
		profilerTasks.push_back(task);
		return taskId;
	}

	ProfilerTask CpuProfiler::EndTask(size_t taskId)
	{
		assert(profilerTasks.size() == taskId + 1 && profilerTasks.back().endTime < 0.0);
		profilerTasks.back().endTime = GetCurrFrameTimeSeconds();
		return profilerTasks.back();
	}

	size_t CpuProfiler::StartFrame()
	{
		profilerTasks.clear();
		frameStartTime = hrc::now();
		return frameIndex;
	}

	void CpuProfiler::EndFrame(size_t frameId)
	{
		assert(frameId == frameIndex);
		frameIndex++;
	}

	const std::vector<ProfilerTask>& CpuProfiler::GetProfilerTasks()
	{
		return profilerTasks;
	}

	double CpuProfiler::GetCurrFrameTimeSeconds()
	{
		return double(std::chrono::duration_cast<std::chrono::microseconds>(hrc::now() - frameStartTime).count()) / 1e6;
	}

	CpuProfiler::TaskHandleInfo::TaskHandleInfo(CpuProfiler* _profiler, size_t _taskId)
	{
		this->profiler = _profiler;
		this->taskId = _taskId;
	}

	void CpuProfiler::TaskHandleInfo::Reset()
	{
		profiler->EndTask(taskId);
	}

	CpuProfiler::FrameHandleInfo::FrameHandleInfo(CpuProfiler* _profiler, size_t _frameId)
	{
		this->profiler = _profiler;
		this->frameId = _frameId;
	}

	void CpuProfiler::FrameHandleInfo::Reset()
	{
		profiler->EndFrame(frameId);
	}

	CpuProfiler::ScopedTask CpuProfiler::StartScopedTask(std::string taskName, uint32_t taskColor)
	{
		return ScopedTask(TaskHandleInfo(this, StartTask(taskName, taskColor)), true);
	}

	CpuProfiler::ScopedFrame CpuProfiler::StartScopedFrame()
	{
		return ScopedFrame(FrameHandleInfo(this, StartFrame()), true);
	}
}
