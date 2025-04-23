#include "GpuProfiler.h"

namespace lz
{
	GpuProfiler::GpuProfiler(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice, uint32_t maxTimestampsCount):
		logicalDevice(logicalDevice),
		timestampQuery(physicalDevice, logicalDevice, maxTimestampsCount)
	{
		frameIndex = 0;
	}

	size_t GpuProfiler::StartTask(std::string taskName, uint32_t taskColor,
		vk::PipelineStageFlagBits pipelineStageFlags)
	{
		timestampQuery.AddTimestamp(frameCommandBuffer, profilerTasks.size(), pipelineStageFlags);

		lz::ProfilerTask task;
		task.color = taskColor;
		task.name = taskName;
		task.startTime = -1.0;
		task.endTime = -1.0;
		size_t taskId = profilerTasks.size();
		profilerTasks.push_back(task);


		return taskId;
	}

	void GpuProfiler::EndTask(size_t taskId)
	{
		assert(profilerTasks.size() == taskId + 1 && profilerTasks.back().endTime < 0.0);
	}

	size_t GpuProfiler::StartFrame(vk::CommandBuffer commandBuffer)
	{
		this->frameCommandBuffer = commandBuffer;
		profilerTasks.clear();
		timestampQuery.ResetQueryPool(frameCommandBuffer);
		return frameIndex;
	}

	void GpuProfiler::EndFrame(size_t frameId)
	{
		timestampQuery.AddTimestamp(frameCommandBuffer, profilerTasks.size(),
		                            vk::PipelineStageFlagBits::eBottomOfPipe);

		assert(frameId == frameIndex);
		frameIndex++;
	}

	const std::vector<ProfilerTask>& GpuProfiler::GetProfilerTasks()
	{
		return profilerTasks;
	}

	GpuProfiler::TaskHandleInfo::TaskHandleInfo(GpuProfiler* _profiler, size_t _taskId)
	{
		this->profiler = _profiler;
		this->taskId = _taskId;
	}

	void GpuProfiler::TaskHandleInfo::Reset()
	{
		profiler->EndTask(taskId);
	}

	GpuProfiler::FrameHandleInfo::FrameHandleInfo(GpuProfiler* _profiler, size_t _frameId)
	{
		this->profiler = _profiler;
		this->frameId = _frameId;
	}

	void GpuProfiler::FrameHandleInfo::Reset()
	{
		profiler->EndFrame(frameId);
	}

	const std::vector<lz::ProfilerTask>& GpuProfiler::GetProfilerData()
	{
		return profilerTasks;
	}

	void GpuProfiler::GatherTimestamps()
	{
		if (profilerTasks.size() > 0)
		{
			lz::TimestampQuery::QueryResult res = timestampQuery.QueryResults(logicalDevice);
			assert(res.size == this->profilerTasks.size() + 1); //1 is because of end-of-frame timestamp

			for (size_t taskIndex = 0; taskIndex < profilerTasks.size(); taskIndex++)
			{
				auto& task = profilerTasks[taskIndex];
				task.startTime = res.data[taskIndex].time;
				task.endTime = res.data[taskIndex + 1].time;
			}
		}
	}
}
