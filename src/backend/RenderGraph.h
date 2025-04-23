#pragma once

#include <functional>

#include "Image.h"
#include "LingzeVK.h"

#include "RenderPassCache.h"
#include "Pool.h"
#include "Handles.h"
#include "Synchronization.h"
#include "CpuProfiler.h"

namespace lz
{
	class GpuProfiler;
	class Buffer;
	class RenderGraph;

	template <typename Base>
	struct RenderGraphProxyId
	{
		RenderGraphProxyId() :
			id(size_t(-1))
		{
		}

		bool operator ==(const RenderGraphProxyId<Base>& other) const
		{
			return this->id == other.id;
		}

		bool IsValid()
		{
			return id != size_t(-1);
		}

	private:
		explicit RenderGraphProxyId(size_t _id) : id(_id)
		{
		}

		size_t id;
		friend class RenderGraph;
	};

	class ImageCache
	{
	public:
		struct ImageKey
		{
			bool operator <(const ImageKey& other) const;

			vk::Format format;
			vk::ImageUsageFlags usageFlags;
			uint32_t mipsCount;
			uint32_t arrayLayersCount;
			glm::uvec3 size;
			std::string debugName;
		};

		ImageCache(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice, vk::DispatchLoaderDynamic loader);

		void Release();

		lz::ImageData* GetImage(ImageKey imageKey);

	private:
		struct ImageCacheEntry
		{
			ImageCacheEntry();

			std::vector<std::unique_ptr<lz::Image>> images;
			size_t usedCount;
		};

		std::map<ImageKey, ImageCacheEntry> imageCache;
		vk::PhysicalDevice physicalDevice;
		vk::Device logicalDevice;
		vk::DispatchLoaderDynamic loader;
	};

	class ImageViewCache
	{
	public:
		struct ImageViewKey
		{
			lz::ImageData* image;
			ImageSubresourceRange subresourceRange;
			std::string debugName;

			bool operator <(const ImageViewKey& other) const;
		};

		ImageViewCache(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice);

		lz::ImageView* GetImageView(ImageViewKey imageViewKey);

	private:
		std::map<ImageViewKey, std::unique_ptr<lz::ImageView>> imageViewCache;
		vk::PhysicalDevice physicalDevice;
		vk::Device logicalDevice;
	};

	class BufferCache
	{
	public:
		BufferCache(vk::PhysicalDevice _physicalDevice, vk::Device _logicalDevice);

		struct BufferKey
		{
			uint32_t elementSize;
			uint32_t elementsCount;

			bool operator <(const BufferKey& other) const;
		};

		void Release();

		lz::Buffer* GetBuffer(BufferKey bufferKey);

	private:
		struct BufferCacheEntry
		{
			BufferCacheEntry();

			std::vector<std::unique_ptr<lz::Buffer>> buffers;
			size_t usedCount;
		};

		std::map<BufferKey, BufferCacheEntry> bufferCache;
		vk::PhysicalDevice physicalDevice;
		vk::Device logicalDevice;
	};

	class RenderGraph
	{
	private:
		struct ImageProxy;
		using ImageProxyPool = Utils::Pool<ImageProxy>;

		struct ImageViewProxy;
		using ImageViewProxyPool = Utils::Pool<ImageViewProxy>;

		struct BufferProxy;
		using BufferProxyPool = Utils::Pool<BufferProxy>;

	public:
		using ImageProxyId = ImageProxyPool::Id;
		using ImageViewProxyId = ImageViewProxyPool::Id;
		using BufferProxyId = BufferProxyPool::Id;

		struct ImageHandleInfo
		{
			ImageHandleInfo();

			ImageHandleInfo(RenderGraph* renderGraph, ImageProxyId imageProxyId);

			void Reset();

			ImageProxyId Id() const;

			void SetDebugName(std::string name) const;

		private:
			RenderGraph* renderGraph;
			ImageProxyId imageProxyId;
			friend class RenderGraph;
		};

		struct ImageViewHandleInfo
		{
			ImageViewHandleInfo();

			ImageViewHandleInfo(RenderGraph* renderGraph, ImageViewProxyId imageViewProxyId);

			void Reset();

			ImageViewProxyId Id() const;

			void SetDebugName(std::string name) const;

		private:
			RenderGraph* renderGraph;
			ImageViewProxyId imageViewProxyId;
			friend class RenderGraph;
		};

		struct BufferHandleInfo
		{
			BufferHandleInfo();

			BufferHandleInfo(RenderGraph* renderGraph, BufferProxyId bufferProxyId);

			void Reset();

			BufferProxyId Id() const;

		private:
			RenderGraph* renderGraph;
			BufferProxyId bufferProxyId;
			friend class RenderGraph;
		};

	public:
		RenderGraph(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice, vk::DispatchLoaderDynamic loader);

		using ImageProxyUnique = UniqueHandle<ImageHandleInfo, RenderGraph>;
		using ImageViewProxyUnique = UniqueHandle<ImageViewHandleInfo, RenderGraph>;
		using BufferProxyUnique = UniqueHandle<BufferHandleInfo, RenderGraph>;

		ImageProxyUnique AddImage(vk::Format format, uint32_t mipsCount, uint32_t arrayLayersCount, glm::uvec2 size,
		                          vk::ImageUsageFlags usageFlags);

		ImageProxyUnique AddImage(vk::Format format, uint32_t mipsCount, uint32_t arrayLayersCount, glm::uvec3 size,
		                          vk::ImageUsageFlags usageFlags);

		ImageProxyUnique AddExternalImage(lz::ImageData* image);

		ImageViewProxyUnique AddImageView(ImageProxyId imageProxyId, uint32_t baseMipLevel, uint32_t mipLevelsCount,
		                                  uint32_t baseArrayLayer, uint32_t arrayLayersCount);

		ImageViewProxyUnique AddExternalImageView(lz::ImageView* imageView,
		                                          lz::ImageUsageTypes usageType = lz::ImageUsageTypes::Unknown);

	private:
		void DeleteImage(ImageProxyId imageId);

		void SetImageProxyDebugName(ImageProxyId imageId, std::string debugName);

		void SetImageViewProxyDebugName(ImageViewProxyId imageViewId, std::string debugName);

		void DeleteImageView(ImageViewProxyId imageViewId);

		void DeleteBuffer(BufferProxyId bufferId);

	public:
		glm::uvec2 GetMipSize(ImageProxyId imageProxyId, uint32_t mipLevel);

		glm::uvec2 GetMipSize(ImageViewProxyId imageViewProxyId, uint32_t mipOffset);

		template <typename BufferType>
		RenderGraph::BufferProxyUnique AddBuffer(uint32_t count)
		{
			BufferProxy bufferProxy;
			bufferProxy.type = BufferProxy::Types::Transient;
			bufferProxy.bufferKey.elementSize = sizeof(BufferType);
			bufferProxy.bufferKey.elementsCount = count;
			bufferProxy.externalBuffer = nullptr;
			return BufferProxyUnique(BufferHandleInfo(this, bufferProxies.Add(std::move(bufferProxy))));
		}

		BufferProxyUnique AddExternalBuffer(lz::Buffer* buffer);

		struct PassContext
		{
			lz::ImageView* GetImageView(ImageViewProxyId imageViewProxyId);

			lz::Buffer* GetBuffer(BufferProxyId bufferProxy);

			vk::CommandBuffer GetCommandBuffer();

		private:
			std::vector<lz::ImageView*> resolvedImageViews;
			std::vector<lz::Buffer*> resolvedBuffers;
			vk::CommandBuffer commandBuffer;
			friend class RenderGraph;
		};

		struct RenderPassContext : public PassContext
		{
			lz::RenderPass* GetRenderPass();

		private:
			lz::RenderPass* renderPass;
			friend class RenderGraph;
		};

		struct RenderPassDesc
		{
			RenderPassDesc();

			struct Attachment
			{
				ImageViewProxyId imageViewProxyId;
				vk::AttachmentLoadOp loadOp;
				vk::ClearValue clearValue;
			};

			RenderPassDesc& SetColorAttachments(
				const std::vector<ImageViewProxyId>& colorAttachmentViewProxies,
				vk::AttachmentLoadOp loadop = vk::AttachmentLoadOp::eDontCare,
				vk::ClearValue clearValue = vk::ClearColorValue(std::array<float, 4>{1.0f, 0.5f, 0.0f, 1.0f}));

			RenderPassDesc& SetColorAttachments(std::vector<Attachment>&& _colorAttachments);

			RenderPassDesc& SetDepthAttachment(
				ImageViewProxyId _depthAttachmentViewProxyId,
				vk::AttachmentLoadOp _loadOp = vk::AttachmentLoadOp::eDontCare,
				vk::ClearValue _clearValue = vk::ClearDepthStencilValue(1.0f, 0));

			RenderPassDesc& SetDepthAttachment(Attachment _depthAttachment);

			RenderPassDesc& SetVertexBuffers(std::vector<BufferProxyId>&& _vertexBufferProxies);

			RenderPassDesc& SetInputImages(std::vector<ImageViewProxyId>&& _inputImageViewProxies);

			RenderPassDesc& SetStorageBuffers(std::vector<BufferProxyId>&& _inoutStorageBufferProxies);

			RenderPassDesc& SetStorageImages(std::vector<ImageViewProxyId>&& _inoutStorageImageProxies);

			RenderPassDesc& SetRenderAreaExtent(vk::Extent2D _renderAreaExtent);

			RenderPassDesc& SetRecordFunc(std::function<void(RenderPassContext)> _recordFunc);

			RenderPassDesc& SetProfilerInfo(uint32_t taskColor, std::string taskName);

			std::vector<Attachment> colorAttachments;
			Attachment depthAttachment;

			std::vector<ImageViewProxyId> inputImageViewProxies;
			std::vector<BufferProxyId> vertexBufferProxies;
			std::vector<BufferProxyId> inoutStorageBufferProxies;
			std::vector<ImageViewProxyId> inoutStorageImageProxies;

			vk::Extent2D renderAreaExtent;
			std::function<void(RenderPassContext)> recordFunc;

			std::string profilerTaskName;
			uint32_t profilerTaskColor;
		};

		void AddRenderPass(
			std::vector<ImageViewProxyId> colorAttachmentImageProxies,
			ImageViewProxyId depthAttachmentImageProxy,
			std::vector<ImageViewProxyId> inputImageViewProxies,
			vk::Extent2D renderAreaExtent,
			vk::AttachmentLoadOp loadOp,
			std::function<void(RenderPassContext)> recordFunc);

		void AddPass(RenderPassDesc& renderPassDesc);

		void Clear();

		struct ComputePassDesc
		{
			ComputePassDesc();

			ComputePassDesc& SetInputImages(std::vector<ImageViewProxyId>&& _inputImageViewProxies);

			ComputePassDesc& SetStorageBuffers(std::vector<BufferProxyId>&& _inoutStorageBufferProxies);

			ComputePassDesc& SetStorageImages(std::vector<ImageViewProxyId>&& _inoutStorageImageProxies);

			ComputePassDesc& SetRecordFunc(std::function<void(PassContext)> _recordFunc);

			ComputePassDesc& SetProfilerInfo(uint32_t taskColor, std::string taskName);

			std::vector<BufferProxyId> inoutStorageBufferProxies;
			std::vector<ImageViewProxyId> inputImageViewProxies;
			std::vector<ImageViewProxyId> inoutStorageImageProxies;

			std::function<void(PassContext)> recordFunc;

			std::string profilerTaskName;
			uint32_t profilerTaskColor;
		};

		void AddComputePass(
			std::vector<BufferProxyId> inoutBufferProxies,
			std::vector<ImageViewProxyId> inputImageViewProxies,
			std::function<void(PassContext)> recordFunc);

		void AddPass(ComputePassDesc& computePassDesc);


		struct TransferPassDesc
		{
			TransferPassDesc();

			TransferPassDesc& SetSrcImages(std::vector<ImageViewProxyId>&& _srcImageViewProxies);

			TransferPassDesc& SetDstImages(std::vector<ImageViewProxyId>&& _dstImageViewProxies);

			TransferPassDesc& SetSrcBuffers(std::vector<BufferProxyId>&& _srcBufferProxies);

			TransferPassDesc& SetDstBuffers(std::vector<BufferProxyId>&& _dstBufferProxies);

			TransferPassDesc& SetRecordFunc(std::function<void(PassContext)> _recordFunc);

			TransferPassDesc& SetProfilerInfo(uint32_t taskColor, std::string taskName);

			std::vector<BufferProxyId> srcBufferProxies;
			std::vector<ImageViewProxyId> srcImageViewProxies;

			std::vector<BufferProxyId> dstBufferProxies;
			std::vector<ImageViewProxyId> dstImageViewProxies;

			std::function<void(PassContext)> recordFunc;

			std::string profilerTaskName;
			uint32_t profilerTaskColor;
		};

		void AddPass(TransferPassDesc& transferPassDesc);

		struct ImagePresentPassDesc
		{
			ImagePresentPassDesc& SetImage(ImageViewProxyId _presentImageViewProxyId);

			ImageViewProxyId presentImageViewProxyId;
		};

		void AddPass(ImagePresentPassDesc&& imagePresentDesc);

		void AddImagePresent(ImageViewProxyId presentImageViewProxyId);

		struct FrameSyncBeginPassDesc
		{
		};

		struct FrameSyncEndPassDesc
		{
		};

		void AddPass(FrameSyncBeginPassDesc&& frameSyncBeginDesc);

		void AddPass(FrameSyncEndPassDesc&& frameSyncEndDesc);

		void Execute(vk::CommandBuffer commandBuffer, lz::CpuProfiler* cpuProfiler, lz::GpuProfiler* gpuProfiler);

	private:
		void FlushExternalImages(vk::CommandBuffer commandBuffer, lz::CpuProfiler* cpuProfiler,
		                         lz::GpuProfiler* gpuProfiler);

		bool ImageViewContainsSubresource(lz::ImageView* imageView, lz::ImageData* imageData, uint32_t mipLevel,
		                                  uint32_t arrayLayer);

		ImageUsageTypes GetTaskImageSubresourceUsageType(size_t taskIndex, lz::ImageData* imageData, uint32_t mipLevel,
		                                                 uint32_t arrayLayer);

		BufferUsageTypes GetTaskBufferUsageType(size_t taskIndex, lz::Buffer* buffer);

		ImageUsageTypes GetLastImageSubresourceUsageType(size_t taskIndex, lz::ImageData* imageData, uint32_t mipLevel,
		                                                 uint32_t arrayLayer);

		BufferUsageTypes GetLastBufferUsageType(size_t taskIndex, lz::Buffer* buffer);

		void FlushImageTransitionBarriers(lz::ImageData* imageData, vk::ImageSubresourceRange range,
		                                  ImageUsageTypes srcUsageType, ImageUsageTypes dstUsageType,
		                                  vk::PipelineStageFlags& srcStage, vk::PipelineStageFlags& dstStage,
		                                  std::vector<vk::ImageMemoryBarrier>& imageBarriers);

		void AddImageTransitionBarriers(lz::ImageView* imageView, ImageUsageTypes dstUsageType, size_t dstTaskIndex,
		                                vk::PipelineStageFlags& srcStage, vk::PipelineStageFlags& dstStage,
		                                std::vector<vk::ImageMemoryBarrier>& imageBarriers);

		void FlushBufferTransitionBarriers(lz::Buffer* buffer, BufferUsageTypes srcUsageType,
		                                   BufferUsageTypes dstUsageType, vk::PipelineStageFlags& srcStage,
		                                   vk::PipelineStageFlags& dstStage,
		                                   std::vector<vk::BufferMemoryBarrier>& bufferBarriers);

		void AddBufferBarriers(lz::Buffer* buffer, BufferUsageTypes dstUsageType, size_t dstTaskIndex,
		                       vk::PipelineStageFlags& srcStage, vk::PipelineStageFlags& dstStage,
		                       std::vector<vk::BufferMemoryBarrier>& bufferBarriers);

	private:
		struct ImageProxy
		{
			enum struct Types
			{
				Transient,
				External
			};

			ImageCache::ImageKey imageKey;
			lz::ImageData* externalImage;

			lz::ImageData* resolvedImage;

			Types type;
		};

		struct ImageViewProxy
		{
			enum struct Types
			{
				Transient,
				External
			};

			bool Contains(const ImageViewProxy& other);

			ImageProxyId imageProxyId;
			ImageSubresourceRange subresourceRange;

			lz::ImageView* externalView;
			lz::ImageUsageTypes externalUsageType;

			lz::ImageView* resolvedImageView;
			std::string debugName;

			Types type;
		};

		struct BufferProxy
		{
			enum struct Types
			{
				Transient,
				External
			};

			BufferCache::BufferKey bufferKey;
			lz::Buffer* externalBuffer;
			lz::Buffer* resolvedBuffer;

			Types type;
		};

		struct Task
		{
			enum struct Types
			{
				RenderPass,
				ComputePass,
				TransferPass,
				ImagePresent,
				FrameSyncBegin,
				FrameSyncEnd
			};

			Types type;
			size_t index;
		};

		ImageCache imageCache;
		ImageProxyPool imageProxies;

		void ResolveImages();

		lz::ImageData* GetResolvedImage(size_t taskIndex, ImageProxyId imageProxy);

		ImageViewCache imageViewCache;
		ImageViewProxyPool imageViewProxies;

		void ResolveImageViews();

		lz::ImageView* GetResolvedImageView(size_t taskIndex, ImageViewProxyId imageViewProxyId);


		BufferCache bufferCache;
		BufferProxyPool bufferProxies;

		void ResolveBuffers();

		lz::Buffer* GetResolvedBuffer(size_t taskIndex, BufferProxyId bufferProxyId);

		std::vector<Task> tasks;

		void AddTask(Task task);

		lz::ProfilerTask CreateProfilerTask(const RenderPassDesc& renderPassDesc);

		lz::ProfilerTask CreateProfilerTask(const ComputePassDesc& computePassDesc);

		lz::ProfilerTask CreateProfilerTask(const TransferPassDesc& transferPassDesc);

		lz::ProfilerTask CreateProfilerTask(const ImagePresentPassDesc& imagePresentPassDesc);

		lz::ProfilerTask CreateProfilerTask(const FrameSyncBeginPassDesc& frameSyncBeginPassDesc);

		lz::ProfilerTask CreateProfilerTask(const FrameSyncEndPassDesc& frameSyncEndPassDesc);

		RenderPassCache renderPassCache;
		FramebufferCache framebufferCache;

		std::vector<RenderPassDesc> renderPassDescs;
		std::vector<ComputePassDesc> computePassDescs;
		std::vector<TransferPassDesc> transferPassDescs;
		std::vector<ImagePresentPassDesc> imagePresentDescs;
		std::vector<FrameSyncBeginPassDesc> frameSyncBeginDescs;
		std::vector<FrameSyncEndPassDesc> frameSyncEndDescs;


		vk::Device logicalDevice;
		vk::PhysicalDevice physicalDevice;
		vk::DispatchLoaderDynamic loader;
		size_t imageAllocations = 0;
	};
}
