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
        PresentQueue(lz::Core* core, lz::WindowDesc windowDesc, uint32_t imagesCount, vk::PresentModeKHR preferredMode);

        void RecreateSwapchain();

        lz::ImageView* AcquireImage(vk::Semaphore signalSemaphore);

        void PresentImage(vk::Semaphore waitSemaphore);

        vk::Extent2D GetImageSize();

    private:
        lz::Core* core;
        std::unique_ptr<lz::Swapchain> swapchain;
        
        lz::WindowDesc windowDesc;
        uint32_t imagesCount;
        vk::PresentModeKHR preferredMode;

        std::vector<lz::ImageView*> swapchainImageViews;
        uint32_t imageIndex;

        vk::Rect2D swapchainRect;
    };

    

    struct InFlightQueue
    {
        InFlightQueue(lz::Core* core, lz::WindowDesc windowDesc, uint32_t inFlightCount, vk::PresentModeKHR preferredMode);

        // 重建交换链
        void RecreateSwapchain();

        void InitFrameResources();

        vk::Extent2D GetImageSize();

        size_t GetInFlightFramesCount();

        struct FrameInfo
        {
            lz::ShaderMemoryPool* memoryPool;
            size_t frameIndex;
            lz::RenderGraph::ImageViewProxyId swapchainImageViewProxyId;
        };

        FrameInfo BeginFrame();

        void EndFrame();

        const std::vector<lz::ProfilerTask>& GetLastFrameCpuProfilerData();

        const std::vector<lz::ProfilerTask>& GetLastFrameGpuProfilerData();

        CpuProfiler& GetCpuProfiler();

    private:
        std::unique_ptr<lz::ShaderMemoryPool> memoryPool;
        std::map<lz::ImageView*, lz::RenderGraph::ImageViewProxyUnique> swapchainImageViewProxies;
        
        lz::WindowDesc windowDesc;
        uint32_t inFlightCount;
        vk::PresentModeKHR preferredMode;

        struct FrameResources
        {
            vk::UniqueSemaphore imageAcquiredSemaphore;
            vk::UniqueSemaphore renderingFinishedSemaphore;
            vk::UniqueFence inFlightFence;

            vk::UniqueCommandBuffer commandBuffer;
            std::unique_ptr<lz::Buffer> shaderMemoryBuffer;
            std::unique_ptr<lz::GpuProfiler> gpuProfiler;
        };
        std::vector<FrameResources> frames;
        size_t frameIndex;

        lz::Core* core;
        lz::ImageView* currSwapchainImageView;
        std::unique_ptr<PresentQueue> presentQueue;
        lz::CpuProfiler cpuProfiler;
        std::vector<lz::ProfilerTask> lastFrameCpuProfilerTasks;

        size_t profilerFrameId;
    };

    

    struct ExecuteOnceQueue
    {
        ExecuteOnceQueue(lz::Core* core);

        vk::CommandBuffer BeginCommandBuffer();

        void EndCommandBuffer();

    private:
        lz::Core* core;
        vk::UniqueCommandBuffer commandBuffer;
    };

    
}
