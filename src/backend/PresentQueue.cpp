#include "PresentQueue.h"

#include "Buffer.h"
#include "Core.h"
#include "ShaderMemoryPool.h"
#include "Swapchain.h"

namespace lz
{
	PresentQueue::PresentQueue(lz::Core* core, lz::WindowDesc windowDesc, uint32_t imagesCount,
	                           vk::PresentModeKHR preferredMode)
	{
		this->core = core;
		this->windowDesc = windowDesc;
		this->imagesCount = imagesCount;
		this->preferredMode = preferredMode;

		RecreateSwapchain();
	}

	void PresentQueue::RecreateSwapchain()
	{
		// 完全重新创建交换链，而不是重用现有的交换链
		if (this->swapchain)
		{
			// 首先尝试使用更优雅的方式重建交换链
			int width, height;
			RECT rect;
			GetClientRect(windowDesc.hWnd, &rect);
			width = rect.right - rect.left;
			height = rect.bottom - rect.top;

			if (width > 0 && height > 0)
			{
				vk::Extent2D newSize(width, height);
				bool recreateSuccess = swapchain->Recreate(newSize);
				if (recreateSuccess)
				{
					// 重建成功
					this->swapchainImageViews = swapchain->GetImageViews();
					this->swapchainRect = vk::Rect2D(vk::Offset2D(), swapchain->GetSize());
					this->imageIndex = -1;
					return;
				}
			}

			// 如果重建失败，则释放旧的交换链，创建新的
			this->swapchain.reset();
		}

		// 创建全新的交换链
		this->swapchain = core->CreateSwapchain(windowDesc, imagesCount, preferredMode);
		this->swapchainImageViews = swapchain->GetImageViews();
		this->swapchainRect = vk::Rect2D(vk::Offset2D(), swapchain->GetSize());
		this->imageIndex = -1;
	}

	lz::ImageView* PresentQueue::AcquireImage(vk::Semaphore signalSemaphore)
	{
		this->imageIndex = swapchain->AcquireNextImage(signalSemaphore).value;
		return swapchainImageViews[imageIndex];
	}

	void PresentQueue::PresentImage(vk::Semaphore waitSemaphore)
	{
		vk::SwapchainKHR swapchains[] = {swapchain->GetHandle()};
		vk::Semaphore waitSemaphores[] = {waitSemaphore};
		auto presentInfo = vk::PresentInfoKHR()
		                   .setSwapchainCount(1)
		                   .setPSwapchains(swapchains)
		                   .setPImageIndices(&imageIndex)
		                   .setPResults(nullptr)
		                   .setWaitSemaphoreCount(1)
		                   .setPWaitSemaphores(waitSemaphores);

		auto res = core->GetPresentQueue().presentKHR(presentInfo);
	}

	vk::Extent2D PresentQueue::GetImageSize()
	{
		return swapchain->GetSize();
	}

	InFlightQueue::InFlightQueue(lz::Core* core, lz::WindowDesc windowDesc, uint32_t inFlightCount,
	                             vk::PresentModeKHR preferredMode)
	{
		this->core = core;
		this->windowDesc = windowDesc;
		this->inFlightCount = inFlightCount;
		this->preferredMode = preferredMode;
		this->memoryPool = std::make_unique<lz::ShaderMemoryPool>(core->GetDynamicMemoryAlignment());

		presentQueue.reset(new PresentQueue(core, windowDesc, inFlightCount, preferredMode));
		InitFrameResources();
	}

	void InFlightQueue::RecreateSwapchain()
	{
		// 等待设备空闲
		core->WaitIdle();

		// 删除所有与旧交换链相关的资源
		swapchainImageViewProxies.clear();

		// 重新创建交换链
		presentQueue->RecreateSwapchain();
	}

	void InFlightQueue::InitFrameResources()
	{
		frames.clear();
		for (size_t frameIndex = 0; frameIndex < inFlightCount; frameIndex++)
		{
			FrameResources frame;
			frame.inFlightFence = core->CreateFence(true);
			frame.imageAcquiredSemaphore = core->CreateVulkanSemaphore();
			frame.renderingFinishedSemaphore = core->CreateVulkanSemaphore();

			frame.commandBuffer = std::move(core->AllocateCommandBuffers(1)[0]);
			core->SetObjectDebugName(frame.commandBuffer.get(),
			                         std::string("Frame") + std::to_string(frameIndex) + " command buffer");
			frame.shaderMemoryBuffer = std::unique_ptr<lz::Buffer>(new lz::Buffer(
				core->GetPhysicalDevice(), core->GetLogicalDevice(), 100000000, vk::BufferUsageFlagBits::eUniformBuffer,
				vk::MemoryPropertyFlagBits::eHostCoherent));
			frame.gpuProfiler = std::unique_ptr<lz::GpuProfiler>(
				new lz::GpuProfiler(core->GetPhysicalDevice(), core->GetLogicalDevice(), 512));
			frames.push_back(std::move(frame));
		}
		frameIndex = 0;
	}

	vk::Extent2D InFlightQueue::GetImageSize()
	{
		return presentQueue->GetImageSize();
	}

	size_t InFlightQueue::GetInFlightFramesCount()
	{
		return frames.size();
	}

	InFlightQueue::FrameInfo InFlightQueue::BeginFrame()
	{
		this->profilerFrameId = cpuProfiler.StartFrame();

		auto& currFrame = frames[frameIndex];
		{
			auto fenceTask = cpuProfiler.StartScopedTask("WaitForFence", lz::Colors::pomegranate);
			core->WaitForFence(currFrame.inFlightFence.get());
			core->ResetFence(currFrame.inFlightFence.get());
		}

		{
			auto imageAcquireTask = cpuProfiler.StartScopedTask("ImageAcquire", lz::Colors::emerald);
			currSwapchainImageView = presentQueue->AcquireImage(currFrame.imageAcquiredSemaphore.get());
		}

		{
			auto gpuGatheringTask = cpuProfiler.StartScopedTask("GpuPrfGathering", lz::Colors::amethyst);
			currFrame.gpuProfiler->GatherTimestamps();
		}

		auto& swapchainViewProxyId = swapchainImageViewProxies[currSwapchainImageView];
		if (!swapchainViewProxyId.IsAttached())
		{
			swapchainViewProxyId = core->GetRenderGraph()->AddExternalImageView(currSwapchainImageView);
		}
		core->GetRenderGraph()->AddPass(lz::RenderGraph::FrameSyncBeginPassDesc());

		memoryPool->MapBuffer(currFrame.shaderMemoryBuffer.get());

		FrameInfo frameInfo;
		frameInfo.memoryPool = memoryPool.get();
		frameInfo.frameIndex = frameIndex;
		frameInfo.swapchainImageViewProxyId = swapchainViewProxyId->Id();

		return frameInfo;
	}

	void InFlightQueue::EndFrame()
	{
		auto& currFrame = frames[frameIndex];

		core->GetRenderGraph()->AddImagePresent(swapchainImageViewProxies[currSwapchainImageView]->Id());
		core->GetRenderGraph()->AddPass(lz::RenderGraph::FrameSyncEndPassDesc());

		auto bufferBeginInfo = vk::CommandBufferBeginInfo()
			.setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
		currFrame.commandBuffer->begin(bufferBeginInfo);
		{
			auto gpuFrame = currFrame.gpuProfiler->StartScopedFrame(currFrame.commandBuffer.get());
			core->GetRenderGraph()->Execute(currFrame.commandBuffer.get(), &cpuProfiler, currFrame.gpuProfiler.get());
		}
		currFrame.commandBuffer->end();

		memoryPool->UnmapBuffer();


		{
			{
				auto presentTask = cpuProfiler.StartScopedTask("Submit", lz::Colors::amethyst);
				vk::Semaphore waitSemaphores[] = {currFrame.imageAcquiredSemaphore.get()};
				vk::Semaphore signalSemaphores[] = {currFrame.renderingFinishedSemaphore.get()};
				vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};

				auto submitInfo = vk::SubmitInfo()
				                  .setWaitSemaphoreCount(1)
				                  .setPWaitSemaphores(waitSemaphores)
				                  .setPWaitDstStageMask(waitStages)
				                  .setCommandBufferCount(1)
				                  .setPCommandBuffers(&currFrame.commandBuffer.get())
				                  .setSignalSemaphoreCount(1)
				                  .setPSignalSemaphores(signalSemaphores);

				core->GetGraphicsQueue().submit({submitInfo}, currFrame.inFlightFence.get());
			}
			auto presentTask = cpuProfiler.StartScopedTask("Present", lz::Colors::alizarin);
			presentQueue->PresentImage(currFrame.renderingFinishedSemaphore.get());
		}
		frameIndex = (frameIndex + 1) % frames.size();

		cpuProfiler.EndFrame(profilerFrameId);
		lastFrameCpuProfilerTasks = cpuProfiler.GetProfilerTasks();
	}

	const std::vector<lz::ProfilerTask>& InFlightQueue::GetLastFrameCpuProfilerData()
	{
		return lastFrameCpuProfilerTasks;
	}

	const std::vector<lz::ProfilerTask>& InFlightQueue::GetLastFrameGpuProfilerData()
	{
		return frames[frameIndex].gpuProfiler->GetProfilerTasks();
	}

	CpuProfiler& InFlightQueue::GetCpuProfiler()
	{
		return cpuProfiler;
	}

	ExecuteOnceQueue::ExecuteOnceQueue(lz::Core* core)
	{
		this->core = core;
		commandBuffer = std::move(core->AllocateCommandBuffers(1)[0]);
	}

	vk::CommandBuffer ExecuteOnceQueue::BeginCommandBuffer()
	{
		auto bufferBeginInfo = vk::CommandBufferBeginInfo()
			.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		commandBuffer->begin(bufferBeginInfo);
		return commandBuffer.get();
	}

	void ExecuteOnceQueue::EndCommandBuffer()
	{
		commandBuffer->end();
		vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eAllCommands};

		auto submitInfo = vk::SubmitInfo()
		                  .setWaitSemaphoreCount(0)
		                  .setPWaitDstStageMask(waitStages)
		                  .setCommandBufferCount(1)
		                  .setPCommandBuffers(&commandBuffer.get())
		                  .setSignalSemaphoreCount(0);

		core->GetGraphicsQueue().submit({submitInfo}, nullptr);
		core->GetGraphicsQueue().waitIdle();
	}
}
