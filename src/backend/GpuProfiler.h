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
		GpuProfiler(vk::PhysicalDevice physical_device, vk::Device logical_device, uint32_t max_timestamps_count);

		size_t start_task(const std::string& task_name, uint32_t task_color, vk::PipelineStageFlagBits pipeline_stage_flags);

		void end_task(size_t task_id) const;

		size_t start_frame(vk::CommandBuffer command_buffer);

		void end_frame(size_t frame_id);

		const std::vector<ProfilerTask>& get_profiler_tasks();

	private:
		struct TaskHandleInfo
		{
			TaskHandleInfo(GpuProfiler* profiler, size_t task_id);

			void reset() const;

			GpuProfiler* profiler;
			size_t task_id;
		};

		struct FrameHandleInfo
		{
			FrameHandleInfo(GpuProfiler* profiler, size_t frame_id);

			void reset() const;

			GpuProfiler* profiler;
			size_t frame_id;
		};

	public:
		using scoped_task = UniqueHandle<TaskHandleInfo, GpuProfiler>;

		scoped_task start_scoped_task(const std::string& task_name, uint32_t task_color,
		                           vk::PipelineStageFlagBits pipeline_stage_flags);

		using scoped_frame = UniqueHandle<FrameHandleInfo, GpuProfiler>;

		scoped_frame start_scoped_frame(vk::CommandBuffer command_buffer);

		const std::vector<lz::ProfilerTask>& get_profiler_data();

		void gather_timestamps();

	private:
		vk::Device logical_device_;
		TimestampQuery timestamp_query_;
		size_t frame_index_;
		std::vector<lz::ProfilerTask> profiler_tasks_;
		vk::CommandBuffer frame_command_buffer_;
		friend struct UniqueHandle<TaskHandleInfo, GpuProfiler>;
	};
}
