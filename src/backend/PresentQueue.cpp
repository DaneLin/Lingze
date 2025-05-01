#include "PresentQueue.h"

#include "Buffer.h"
#include "Core.h"
#include "ShaderMemoryPool.h"
#include "Swapchain.h"

namespace lz
{
PresentQueue::PresentQueue(lz::Core *core, lz::WindowDesc window_desc, uint32_t images_count,
                           vk::PresentModeKHR preferred_mode)
{
	this->core_           = core;
	this->window_desc_    = window_desc;
	this->images_count_   = images_count;
	this->preferred_mode_ = preferred_mode;

	recreate_swapchain();
}

void PresentQueue::recreate_swapchain()
{
	// Completely recreate the swapchain, instead of reusing the existing one
	if (this->swapchain_)
	{
		// First try to rebuild the swapchain in a more elegant way
		RECT rect;
		GetClientRect(window_desc_.h_wnd, &rect);
		int width  = rect.right - rect.left;
		int height = rect.bottom - rect.top;

		if (width > 0 && height > 0)
		{
			const vk::Extent2D new_size(width, height);
			const bool         recreate_success = swapchain_->recreate(new_size);
			if (recreate_success)
			{
				// Rebuild successful
				this->swapchain_image_views_ = swapchain_->get_image_views();
				this->swapchain_rect_        = vk::Rect2D(vk::Offset2D(), swapchain_->get_size());
				this->image_index_           = -1;
				return;
			}
		}

		// If rebuild fails, release the old swapchain and create a new one
		this->swapchain_.reset();
	}

	// Create a brand new swapchain
	this->swapchain_             = core_->create_swapchain(window_desc_, images_count_, preferred_mode_);
	this->swapchain_image_views_ = swapchain_->get_image_views();
	this->swapchain_rect_        = vk::Rect2D(vk::Offset2D(), swapchain_->get_size());
	this->image_index_           = -1;
}

lz::ImageView *PresentQueue::acquire_image(vk::Semaphore signal_semaphore)
{
	this->image_index_ = swapchain_->acquire_next_image(signal_semaphore).value;
	return swapchain_image_views_[image_index_];
}

void PresentQueue::present_image(vk::Semaphore wait_semaphore)
{
	const vk::SwapchainKHR swapchains[]      = {swapchain_->get_handle()};
	const vk::Semaphore    wait_semaphores[] = {wait_semaphore};
	const auto             present_info      = vk::PresentInfoKHR()
	                              .setSwapchainCount(1)
	                              .setPSwapchains(swapchains)
	                              .setPImageIndices(&image_index_)
	                              .setPResults(nullptr)
	                              .setWaitSemaphoreCount(1)
	                              .setPWaitSemaphores(wait_semaphores);

	auto res = core_->get_present_queue().presentKHR(present_info);
}

vk::Extent2D PresentQueue::get_image_size()
{
	return swapchain_->get_size();
}

InFlightQueue::InFlightQueue(lz::Core *core, lz::WindowDesc window_desc, uint32_t in_flight_count,
                             vk::PresentModeKHR preferred_mode)
{
	this->core_            = core;
	this->window_desc_     = window_desc;
	this->in_flight_count_ = in_flight_count;
	this->preferred_mode_  = preferred_mode;
	this->memory_pool_     = std::make_unique<lz::ShaderMemoryPool>(core->get_dynamic_memory_alignment());

	present_queue_.reset(new PresentQueue(core, window_desc, in_flight_count, preferred_mode));
	init_frame_resources();
}

void InFlightQueue::recreate_swapchain()
{
	// Wait for the device to be idle
	core_->wait_idle();

	// Delete all resources related to the old swapchain
	swapchain_image_view_proxies_.clear();

	// Recreate the swapchain
	present_queue_->recreate_swapchain();
}

void InFlightQueue::init_frame_resources()
{
	frames_.clear();
	for (size_t frame_index = 0; frame_index < in_flight_count_; frame_index++)
	{
		FrameResources frame;
		frame.in_flight_fence              = core_->create_fence(true);
		frame.image_acquired_semaphore     = core_->create_vulkan_semaphore();
		frame.rendering_finished_semaphore = core_->create_vulkan_semaphore();

		frame.command_buffer = std::move(core_->allocate_command_buffers(1)[0]);
		core_->set_object_debug_name(frame.command_buffer.get(),
		                             std::string("Frame") + std::to_string(frame_index) + " command buffer");
		frame.shader_memory_buffer = std::make_unique<lz::Buffer>(
		    core_->get_physical_device(), core_->get_logical_device(), 100000000,
		    vk::BufferUsageFlagBits::eUniformBuffer,
		    vk::MemoryPropertyFlagBits::eHostCoherent);
		frame.gpu_profiler = std::make_unique<lz::GpuProfiler>(core_->get_physical_device(),
		                                                       core_->get_logical_device(), 512);
		frames_.push_back(std::move(frame));
	}
	frame_index_ = 0;
}

vk::Extent2D InFlightQueue::get_image_size() const
{
	return present_queue_->get_image_size();
}

size_t InFlightQueue::get_in_flight_frames_count() const
{
	return frames_.size();
}

InFlightQueue::FrameInfo InFlightQueue::begin_frame()
{
	this->profiler_frame_id_ = cpu_profiler_.start_frame();

	auto &curr_frame = frames_[frame_index_];
	{
		auto fence_task = cpu_profiler_.start_scoped_task("WaitForFence", lz::Colors::pomegranate);
		core_->wait_for_fence(curr_frame.in_flight_fence.get());
		core_->reset_fence(curr_frame.in_flight_fence.get());
	}

	{
		auto image_acquire_task    = cpu_profiler_.start_scoped_task("ImageAcquire", lz::Colors::emerald);
		curr_swapchain_image_view_ = present_queue_->acquire_image(curr_frame.image_acquired_semaphore.get());
	}

	{
		auto gpu_gathering_task = cpu_profiler_.start_scoped_task("GpuPrfGathering", lz::Colors::amethyst);
		curr_frame.gpu_profiler->gather_timestamps();
	}

	auto &swapchain_view_proxy_id = swapchain_image_view_proxies_[curr_swapchain_image_view_];
	if (!swapchain_view_proxy_id.is_attached())
	{
		swapchain_view_proxy_id = core_->get_render_graph()->add_external_image_view(curr_swapchain_image_view_);
	}
	core_->get_render_graph()->add_pass(lz::RenderGraph::FrameSyncBeginPassDesc());

	memory_pool_->map_buffer(curr_frame.shader_memory_buffer.get());

	FrameInfo frame_info;
	frame_info.memory_pool                   = memory_pool_.get();
	frame_info.frame_index                   = frame_index_;
	frame_info.swapchain_image_view_proxy_id = swapchain_view_proxy_id->id();

	return frame_info;
}

void InFlightQueue::end_frame()
{
	auto &curr_frame = frames_[frame_index_];

	core_->get_render_graph()->add_image_present(swapchain_image_view_proxies_[curr_swapchain_image_view_]->id());
	core_->get_render_graph()->add_pass(lz::RenderGraph::FrameSyncEndPassDesc());

	constexpr auto buffer_begin_info = vk::CommandBufferBeginInfo()
	                                       .setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
	curr_frame.command_buffer->begin(buffer_begin_info);
	{
		auto gpuFrame = curr_frame.gpu_profiler->start_scoped_frame(curr_frame.command_buffer.get());
		core_->get_render_graph()->execute(curr_frame.command_buffer.get(), &cpu_profiler_, curr_frame.gpu_profiler.get());
	}
	curr_frame.command_buffer->end();

	memory_pool_->unmap_buffer();

	{
		{
			auto                             present_task        = cpu_profiler_.start_scoped_task("Submit", lz::Colors::amethyst);
			const vk::Semaphore              wait_semaphores[]   = {curr_frame.image_acquired_semaphore.get()};
			const vk::Semaphore              signal_semaphores[] = {curr_frame.rendering_finished_semaphore.get()};
			constexpr vk::PipelineStageFlags wait_stages[]       = {vk::PipelineStageFlagBits::eColorAttachmentOutput};

			auto submit_info = vk::SubmitInfo()
			                       .setWaitSemaphoreCount(1)
			                       .setPWaitSemaphores(wait_semaphores)
			                       .setPWaitDstStageMask(wait_stages)
			                       .setCommandBufferCount(1)
			                       .setPCommandBuffers(&curr_frame.command_buffer.get())
			                       .setSignalSemaphoreCount(1)
			                       .setPSignalSemaphores(signal_semaphores);

			core_->get_graphics_queue().submit({submit_info}, curr_frame.in_flight_fence.get());
		}
		auto present_task = cpu_profiler_.start_scoped_task("Present", lz::Colors::alizarin);
		present_queue_->present_image(curr_frame.rendering_finished_semaphore.get());
	}
	frame_index_ = (frame_index_ + 1) % frames_.size();

	cpu_profiler_.end_frame(profiler_frame_id_);
	last_frame_cpu_profiler_tasks_ = cpu_profiler_.get_profiler_tasks();
}

const std::vector<lz::ProfilerTask> &InFlightQueue::get_last_frame_cpu_profiler_data()
{
	return last_frame_cpu_profiler_tasks_;
}

const std::vector<lz::ProfilerTask> &InFlightQueue::get_last_frame_gpu_profiler_data()
{
	return frames_[frame_index_].gpu_profiler->get_profiler_tasks();
}

CpuProfiler &InFlightQueue::get_cpu_profiler()
{
	return cpu_profiler_;
}

ExecuteOnceQueue::ExecuteOnceQueue(lz::Core *core)
{
	this->core_     = core;
	command_buffer_ = std::move(core->allocate_command_buffers(1)[0]);
}

vk::CommandBuffer ExecuteOnceQueue::begin_command_buffer()
{
	constexpr auto buffer_begin_info = vk::CommandBufferBeginInfo()
	                                       .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	command_buffer_->begin(buffer_begin_info);
	return command_buffer_.get();
}

void ExecuteOnceQueue::end_command_buffer()
{
	command_buffer_->end();
	constexpr vk::PipelineStageFlags wait_stages[] = {vk::PipelineStageFlagBits::eAllCommands};

	auto submit_info = vk::SubmitInfo()
	                       .setWaitSemaphoreCount(0)
	                       .setPWaitDstStageMask(wait_stages)
	                       .setCommandBufferCount(1)
	                       .setPCommandBuffers(&command_buffer_.get())
	                       .setSignalSemaphoreCount(0);

	core_->get_graphics_queue().submit({submit_info}, nullptr);
	core_->get_graphics_queue().waitIdle();
}
}        // namespace lz
