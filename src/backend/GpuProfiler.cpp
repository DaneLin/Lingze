#include "GpuProfiler.h"

namespace lz
{
	GpuProfiler::GpuProfiler(const vk::PhysicalDevice physical_device, const vk::Device logical_device, const uint32_t max_timestamps_count):
		logical_device_(logical_device),
		timestamp_query_(physical_device, logical_device, max_timestamps_count)
	{
		frame_index_ = 0;
	}

	size_t GpuProfiler::start_task(const std::string& task_name, const uint32_t task_color,
		const vk::PipelineStageFlagBits pipeline_stage_flags)
	{
		timestamp_query_.add_timestamp(frame_command_buffer_, profiler_tasks_.size(), pipeline_stage_flags);

		lz::ProfilerTask task;
		task.color = task_color;
		task.name = task_name;
		task.start_time = -1.0;
		task.end_time = -1.0;
		const size_t task_id = profiler_tasks_.size();
		profiler_tasks_.push_back(task);


		return task_id;
	}

	void GpuProfiler::end_task(const size_t task_id) const
	{
		assert(profiler_tasks_.size() == task_id + 1 && profiler_tasks_.back().end_time < 0.0);
	}

	size_t GpuProfiler::start_frame(const vk::CommandBuffer command_buffer)
	{
		this->frame_command_buffer_ = command_buffer;
		profiler_tasks_.clear();
		timestamp_query_.reset_query_pool(frame_command_buffer_);
		return frame_index_;
	}

	void GpuProfiler::end_frame(const size_t frame_id)
	{
		timestamp_query_.add_timestamp(frame_command_buffer_, profiler_tasks_.size(),
		                            vk::PipelineStageFlagBits::eBottomOfPipe);

		assert(frame_id == frame_index_);
		frame_index_++;
	}

	const std::vector<ProfilerTask>& GpuProfiler::get_profiler_tasks()
	{
		return profiler_tasks_;
	}

	GpuProfiler::TaskHandleInfo::TaskHandleInfo(GpuProfiler* profiler, const size_t task_id)
	{
		this->profiler = profiler;
		this->task_id = task_id;
	}

	void GpuProfiler::TaskHandleInfo::reset() const
	{
		profiler->end_task(task_id);
	}

	GpuProfiler::FrameHandleInfo::FrameHandleInfo(GpuProfiler* profiler, const size_t frame_id)
	{
		this->profiler = profiler;
		this->frame_id = frame_id;
	}

	void GpuProfiler::FrameHandleInfo::reset() const
	{
		profiler->end_frame(frame_id);
	}

	GpuProfiler::ScopedTask GpuProfiler::start_scoped_task(const std::string& task_name, const uint32_t task_color,
		const vk::PipelineStageFlagBits pipeline_stage_flags)
	{
		return ScopedTask(TaskHandleInfo(this, start_task(task_name, task_color, pipeline_stage_flags)), true);
	}

	GpuProfiler::ScopedFrame GpuProfiler::start_scoped_frame(const vk::CommandBuffer command_buffer)
	{
		return ScopedFrame(FrameHandleInfo(this, start_frame(command_buffer)), true);
	}

	const std::vector<lz::ProfilerTask>& GpuProfiler::get_profiler_data()
	{
		return profiler_tasks_;
	}

	void GpuProfiler::gather_timestamps()
	{
		if (!profiler_tasks_.empty())
		{
			const lz::TimestampQuery::QueryResult res = timestamp_query_.query_results(logical_device_);
			assert(res.size == this->profiler_tasks_.size() + 1); //1 is because of end-of-frame timestamp

			for (size_t task_index = 0; task_index < profiler_tasks_.size(); task_index++)
			{
				auto& task = profiler_tasks_[task_index];
				task.start_time = res.data[task_index].time;
				task.end_time = res.data[task_index + 1].time;
			}
		}
	}
}
