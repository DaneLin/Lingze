#pragma once
// Define Win32 platform for Vulkan and prevent min/max macro definitions
#define VK_USE_PLATFORM_WIN32_KHR
#define NOMINMAX
#include <vulkan/vulkan.hpp>

// Include GLM for mathematics operations
#include <glm/glm.hpp>

// Include all backend components
// Core handle types and resource management
#include "Handles.h"
#include "Pool.h"
#include "CpuProfiler.h"
#include "QueueIndices.h"
#include "Surface.h"
#include "ProfilerTask.h"

// Graphics pipeline components
#include "VertexDeclaration.h"
#include "ShaderModule.h"
#include "ShaderProgram.h"
#include "Buffer.h"
#include "Image.h"
#include "Synchronization.h"

// Performance and profiling utilities
#include "TimestampQuery.h"
#include "GpuProfiler.h"

// Resource management
#include "Sampler.h"
#include "Pipeline.h"
#include "ImageView.h"
#include "Swapchain.h"
#include "RenderPass.h"
#include "Framebuffer.h"
#include "ShaderMemoryPool.h"
#include "DescriptorSetCache.h"
#include "PipelineCache.h"
#include "RenderPassCache.h"

// Core engine components
#include "Core.h"
#include "RenderGraph.h"
#include "CoreImpl.h"

// Presentation and resource management
#include "PresentQueue.h"
#include "StagedResources.h"
#include "ImageLoader.h"
