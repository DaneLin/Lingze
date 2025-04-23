#pragma once
#include "Handles.h"
#include <vector>
#include <chrono>
#include <assert.h>

#include "ProfilerTask.h"
#include <string>

namespace lz
{
    class CpuProfiler
    {
    public:
        CpuProfiler();

        size_t StartTask(std::string taskName, uint32_t taskColor);

        ProfilerTask EndTask(size_t taskId);

        size_t StartFrame();

        void EndFrame(size_t frameId);

        const std::vector<ProfilerTask>& GetProfilerTasks();

    private:
        double GetCurrFrameTimeSeconds();

        struct TaskHandleInfo
        {
            TaskHandleInfo(CpuProfiler* _profiler, size_t _taskId);

            void Reset();
            CpuProfiler* profiler;
            size_t taskId;
        };
        struct FrameHandleInfo
        {
            FrameHandleInfo(CpuProfiler* _profiler, size_t _frameId);

            void Reset();
            CpuProfiler* profiler;
            size_t frameId;
        };
    public:
        using ScopedTask = UniqueHandle<TaskHandleInfo, CpuProfiler>;
        ScopedTask StartScopedTask(std::string taskName, uint32_t taskColor);
        using ScopedFrame = UniqueHandle<FrameHandleInfo, CpuProfiler>;
        ScopedFrame StartScopedFrame();

    private:
        using hrc = std::chrono::high_resolution_clock;
        size_t frameIndex;
        std::vector<lz::ProfilerTask> profilerTasks;
        hrc::time_point frameStartTime;
        friend struct UniqueHandle<TaskHandleInfo, CpuProfiler>;
    };
}
