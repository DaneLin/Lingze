#pragma once
#include <map>
#include <Windows.h>

#include "LingzeVK.h"
#include "RenderGraph.h"
#include "Surface.h"
#include "GpuProfiler.h"
#include "ImageView.h"
#include "ShaderMemoryPool.h"
#include "Swapchain.h"

namespace lz
{
	struct PresentQueue
	{
		PresentQueue(lz::Core* core, lz::WindowDesc window_desc, uint32_t images_count,
		             vk::PresentModeKHR preferred_mode);

		void recreate_swapchain();

		lz::ImageView* acquire_image(vk::Semaphore signal_semaphore);

		void present_image(vk::Semaphore wait_semaphore);

		vk::Extent2D get_image_size();

	private:
		lz::Core* core_;
		std::unique_ptr<lz::Swapchain> swapchain_;

		lz::WindowDesc window_desc_;
		uint32_t images_count_;
		vk::PresentModeKHR preferred_mode_;

		std::vector<lz::ImageView*> swapchain_image_views_;
		uint32_t image_index_;

		vk::Rect2D swapchain_rect_;
	};


	struct InFlightQueue
	{
		InFlightQueue(lz::Core* core, lz::WindowDesc window_desc, uint32_t in_flight_count,
		              vk::PresentModeKHR preferred_mode);

		// Recreate swapchain
		void recreate_swapchain();

		void init_frame_resources();

		vk::Extent2D get_image_size() const;

		size_t get_in_flight_frames_count() const;

		struct FrameInfo
		{
			lz::ShaderMemoryPool* memory_pool;
			size_t frame_index;
			lz::RenderGraph::ImageViewProxyId swapchain_image_view_proxy_id;
		};

		FrameInfo begin_frame();

		void end_frame();

		const std::vector<lz::ProfilerTask>& get_last_frame_cpu_profiler_data();

		const std::vector<lz::ProfilerTask>& get_last_frame_gpu_profiler_data();

		CpuProfiler& get_cpu_profiler();

	private:
		std::unique_ptr<lz::ShaderMemoryPool> memory_pool_;
		std::map<lz::ImageView*, lz::RenderGraph::ImageViewProxyUnique> swapchain_image_view_proxies_;

		lz::WindowDesc window_desc_;
		uint32_t in_flight_count_;
		vk::PresentModeKHR preferred_mode_;

		struct FrameResources
		{
			vk::UniqueSemaphore image_acquired_semaphore;
			vk::UniqueSemaphore rendering_finished_semaphore;
			vk::UniqueFence in_flight_fence;

			vk::UniqueCommandBuffer command_buffer;
			std::unique_ptr<lz::Buffer> shader_memory_buffer;
			std::unique_ptr<lz::GpuProfiler> gpu_profiler;
		};

		std::vector<FrameResources> frames_;
		size_t frame_index_;

		lz::Core* core_;
		lz::ImageView* curr_swapchain_image_view_;
		std::unique_ptr<PresentQueue> present_queue_;
		lz::CpuProfiler cpu_profiler_;
		std::vector<lz::ProfilerTask> last_frame_cpu_profiler_tasks_;

		size_t profiler_frame_id_;
	};


	struct ExecuteOnceQueue
	{
		explicit ExecuteOnceQueue(lz::Core* core);

		vk::CommandBuffer begin_command_buffer();

		void end_command_buffer();

	private:
		lz::Core* core_;
		vk::UniqueCommandBuffer command_buffer_;
	};
}
