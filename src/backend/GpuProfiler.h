#pragma once
#include <chrono>

#include "Handles.h"
#include "ProfilerTask.h"
#include "TimestampQuery.h"

namespace lz
{
	class GpuProfiler
	{
	public:
		GpuProfiler(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice, uint32_t maxTimestampsCount);

		size_t StartTask(std::string taskName, uint32_t taskColor, vk::PipelineStageFlagBits pipelineStageFlags);

		void EndTask(size_t taskId);

		size_t StartFrame(vk::CommandBuffer commandBuffer);

		void EndFrame(size_t frameId);

		const std::vector<ProfilerTask>& GetProfilerTasks();

	private:
		struct TaskHandleInfo
		{
			TaskHandleInfo(GpuProfiler* _profiler, size_t _taskId);

			void Reset();

			GpuProfiler* profiler;
			size_t taskId;
		};

		struct FrameHandleInfo
		{
			FrameHandleInfo(GpuProfiler* _profiler, size_t _frameId);

			void Reset();

			GpuProfiler* profiler;
			size_t frameId;
		};

	public:
		using ScopedTask = UniqueHandle<TaskHandleInfo, GpuProfiler>;

		ScopedTask StartScopedTask(std::string taskName, uint32_t taskColor,
		                           vk::PipelineStageFlagBits pipelineStageFlags)
		{
			return ScopedTask(TaskHandleInfo(this, StartTask(taskName, taskColor, pipelineStageFlags)), true);
		}

		using ScopedFrame = UniqueHandle<FrameHandleInfo, GpuProfiler>;

		ScopedFrame StartScopedFrame(vk::CommandBuffer commandBuffer)
		{
			return ScopedFrame(FrameHandleInfo(this, StartFrame(commandBuffer)), true);
		}

		const std::vector<lz::ProfilerTask>& GetProfilerData();

		void GatherTimestamps();

	private:
		vk::Device logicalDevice;
		TimestampQuery timestampQuery;
		size_t frameIndex;
		std::vector<lz::ProfilerTask> profilerTasks;
		vk::CommandBuffer frameCommandBuffer;
		friend struct UniqueHandle<TaskHandleInfo, GpuProfiler>;
	};
}
