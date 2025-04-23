#include "RenderGraph.h"

#include "Buffer.h"
#include "Core.h"
#include "GpuProfiler.h"
#include "ImageView.h"

namespace lz
{
	bool ImageCache::ImageKey::operator<(const ImageKey& other) const
	{
		auto lUsageFlags = VkMemoryMapFlags(usageFlags);
		auto rUsageFlags = VkMemoryMapFlags(other.usageFlags);
		return std::tie(format, mipsCount, arrayLayersCount, lUsageFlags, size.x, size.y, size.z) < std::tie(
			other.format, other.mipsCount, other.arrayLayersCount, rUsageFlags, other.size.x, other.size.y,
			other.size.z);
	}

	ImageCache::ImageCache(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice,
		vk::DispatchLoaderDynamic loader): physicalDevice(physicalDevice),
		                                   logicalDevice(logicalDevice),
		                                   loader(loader)
	{
	}

	void ImageCache::Release()
	{
		for (auto& cacheEntry : imageCache)
		{
			cacheEntry.second.usedCount = 0;
		}
	}

	lz::ImageData* ImageCache::GetImage(ImageKey imageKey)
	{
		auto& cacheEntry = imageCache[imageKey];
		if (cacheEntry.usedCount + 1 > cacheEntry.images.size())
		{
			vk::ImageCreateInfo imageCreateInfo;
			if (imageKey.size.z == glm::u32(-1))
			{
				imageCreateInfo = lz::Image::CreateInfo2d(glm::uvec2(imageKey.size.x, imageKey.size.y),
				                                          imageKey.mipsCount,
				                                          imageKey.arrayLayersCount,
				                                          imageKey.format,
				                                          imageKey.usageFlags);
			}
			else
			{
				imageCreateInfo = lz::Image::CreateInfoVolume(imageKey.size, imageKey.mipsCount,
				                                              imageKey.arrayLayersCount, imageKey.format,
				                                              imageKey.usageFlags);
			}

			auto newImage = std::unique_ptr<lz::Image>(
				new lz::Image(physicalDevice, logicalDevice, imageCreateInfo));
			Core::SetObjectDebugName(logicalDevice, loader, newImage->GetImageData()->GetHandle(),
			                         imageKey.debugName);
			cacheEntry.images.emplace_back(std::move(newImage));
		}
		return cacheEntry.images[cacheEntry.usedCount++]->GetImageData();
	}

	ImageCache::ImageCacheEntry::ImageCacheEntry(): usedCount(0)
	{
	}

	bool ImageViewCache::ImageViewKey::operator<(const ImageViewKey& other) const
	{
		return std::tie(image, subresourceRange) < std::tie(other.image, other.subresourceRange);
	}

	ImageViewCache::ImageViewCache(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice): physicalDevice(physicalDevice), logicalDevice(logicalDevice)
	{
	}

	lz::ImageView* ImageViewCache::GetImageView(ImageViewKey imageViewKey)
	{
		auto& imageView = imageViewCache[imageViewKey];
		if (!imageView)
			imageView = std::unique_ptr<lz::ImageView>(new lz::ImageView(
				logicalDevice,
				imageViewKey.image,
				imageViewKey.subresourceRange.baseMipLevel,
				imageViewKey.subresourceRange.mipsCount,
				imageViewKey.subresourceRange.baseArrayLayer,
				imageViewKey.subresourceRange.arrayLayersCount));
		return imageView.get();
	}

	BufferCache::BufferCache(vk::PhysicalDevice _physicalDevice, vk::Device _logicalDevice): physicalDevice(_physicalDevice),
	                                                                                         logicalDevice(_logicalDevice)
	{
	}

	bool BufferCache::BufferKey::operator<(const BufferKey& other) const
	{
		return std::tie(elementSize, elementsCount) < std::tie(other.elementSize, other.elementsCount);
	}

	void BufferCache::Release()
	{
		for (auto& cacheEntry : bufferCache)
		{
			cacheEntry.second.usedCount = 0;
		}
	}

	lz::Buffer* BufferCache::GetBuffer(BufferKey bufferKey)
	{
		auto& cacheEntry = bufferCache[bufferKey];
		if (cacheEntry.usedCount + 1 > cacheEntry.buffers.size())
		{
			auto newBuffer = std::unique_ptr<lz::Buffer>(new lz::Buffer(
				physicalDevice,
				logicalDevice,
				bufferKey.elementSize * bufferKey.elementsCount,
				vk::BufferUsageFlagBits::eStorageBuffer,
				vk::MemoryPropertyFlagBits::eDeviceLocal));
			cacheEntry.buffers.emplace_back(std::move(newBuffer));
		}
		return cacheEntry.buffers[cacheEntry.usedCount++].get();
	}

	BufferCache::BufferCacheEntry::BufferCacheEntry(): usedCount(0)
	{
	}

	RenderGraph::ImageHandleInfo::ImageHandleInfo()
	{
	}

	RenderGraph::ImageHandleInfo::ImageHandleInfo(RenderGraph* renderGraph, ImageProxyId imageProxyId)
	{
		this->renderGraph = renderGraph;
		this->imageProxyId = imageProxyId;
	}

	void RenderGraph::ImageHandleInfo::Reset()
	{
		renderGraph->DeleteImage(imageProxyId);
	}

	RenderGraph::ImageProxyId RenderGraph::ImageHandleInfo::Id() const
	{
		return imageProxyId;
	}

	void RenderGraph::ImageHandleInfo::SetDebugName(std::string name) const
	{
		renderGraph->SetImageProxyDebugName(imageProxyId, name);
	}

	RenderGraph::ImageViewHandleInfo::ImageViewHandleInfo()
	{
	}

	RenderGraph::ImageViewHandleInfo::ImageViewHandleInfo(RenderGraph* renderGraph, ImageViewProxyId imageViewProxyId)
	{
		this->renderGraph = renderGraph;
		this->imageViewProxyId = imageViewProxyId;
	}

	void RenderGraph::ImageViewHandleInfo::Reset()
	{
		renderGraph->DeleteImageView(imageViewProxyId);
	}

	RenderGraph::ImageViewProxyId RenderGraph::ImageViewHandleInfo::Id() const
	{
		return imageViewProxyId;
	}

	void RenderGraph::ImageViewHandleInfo::SetDebugName(std::string name) const
	{
		renderGraph->SetImageViewProxyDebugName(imageViewProxyId, name);
	}

	RenderGraph::BufferHandleInfo::BufferHandleInfo()
	{
	}

	RenderGraph::BufferHandleInfo::BufferHandleInfo(RenderGraph* renderGraph, BufferProxyId bufferProxyId)
	{
		this->renderGraph = renderGraph;
		this->bufferProxyId = bufferProxyId;
	}

	void RenderGraph::BufferHandleInfo::Reset()
	{
		renderGraph->DeleteBuffer(bufferProxyId);
	}

	RenderGraph::BufferProxyId RenderGraph::BufferHandleInfo::Id() const
	{
		return bufferProxyId;
	}

	RenderGraph::RenderGraph(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice,
		vk::DispatchLoaderDynamic loader): physicalDevice(physicalDevice),
		                                   logicalDevice(logicalDevice),
		                                   loader(loader),
		                                   renderPassCache(logicalDevice),
		                                   framebufferCache(logicalDevice),
		                                   imageCache(physicalDevice, logicalDevice, loader),
		                                   imageViewCache(physicalDevice, logicalDevice),
		                                   bufferCache(physicalDevice, logicalDevice)
	{
	}

	RenderGraph::ImageProxyUnique RenderGraph::AddImage(vk::Format format, uint32_t mipsCount,
		uint32_t arrayLayersCount, glm::uvec2 size, vk::ImageUsageFlags usageFlags)
	{
		return AddImage(format, mipsCount, arrayLayersCount, glm::uvec3(size.x, size.y, -1), usageFlags);
	}

	RenderGraph::ImageProxyUnique RenderGraph::AddImage(vk::Format format, uint32_t mipsCount,
		uint32_t arrayLayersCount, glm::uvec3 size, vk::ImageUsageFlags usageFlags)
	{
		ImageProxy imageProxy;
		imageProxy.type = ImageProxy::Types::Transient;
		imageProxy.imageKey.format = format;
		imageProxy.imageKey.usageFlags = usageFlags;
		imageProxy.imageKey.mipsCount = mipsCount;
		imageProxy.imageKey.arrayLayersCount = arrayLayersCount;
		imageProxy.imageKey.size = size;

		imageProxy.externalImage = nullptr;
		auto uniqueProxyHandle = ImageProxyUnique(ImageHandleInfo(this, imageProxies.Add(std::move(imageProxy))));
		std::string debugName = std::string("Graph image [") + std::to_string(size.x) + ", " +
			std::to_string(size.y) + ", Id=" + std::to_string(uniqueProxyHandle->Id().asInt) + "]" + vk::to_string(
				imageProxy.imageKey.format);
		uniqueProxyHandle->SetDebugName(debugName);
		return uniqueProxyHandle;
	}

	RenderGraph::ImageProxyUnique RenderGraph::AddExternalImage(lz::ImageData* image)
	{
		ImageProxy imageProxy;
		imageProxy.type = ImageProxy::Types::External;
		imageProxy.externalImage = image;
		imageProxy.imageKey.debugName = "External graph image";
		return ImageProxyUnique(ImageHandleInfo(this, imageProxies.Add(std::move(imageProxy))));
	}

	RenderGraph::ImageViewProxyUnique RenderGraph::AddImageView(ImageProxyId imageProxyId, uint32_t baseMipLevel,
		uint32_t mipLevelsCount, uint32_t baseArrayLayer, uint32_t arrayLayersCount)
	{
		ImageViewProxy imageViewProxy;
		imageViewProxy.externalView = nullptr;
		imageViewProxy.type = ImageViewProxy::Types::Transient;
		imageViewProxy.imageProxyId = imageProxyId;
		imageViewProxy.subresourceRange.baseMipLevel = baseMipLevel;
		imageViewProxy.subresourceRange.mipsCount = mipLevelsCount;
		imageViewProxy.subresourceRange.baseArrayLayer = baseArrayLayer;
		imageViewProxy.subresourceRange.arrayLayersCount = arrayLayersCount;
		imageViewProxy.debugName = "View";
		return ImageViewProxyUnique(ImageViewHandleInfo(this, imageViewProxies.Add(std::move(imageViewProxy))));
	}

	RenderGraph::ImageViewProxyUnique RenderGraph::AddExternalImageView(lz::ImageView* imageView,
		lz::ImageUsageTypes usageType)
	{
		ImageViewProxy imageViewProxy;
		imageViewProxy.externalView = imageView;
		imageViewProxy.externalUsageType = usageType;
		imageViewProxy.type = ImageViewProxy::Types::External;
		imageViewProxy.imageProxyId = ImageProxyId();
		imageViewProxy.debugName = "External view";
		return ImageViewProxyUnique(ImageViewHandleInfo(this, imageViewProxies.Add(std::move(imageViewProxy))));
	}

	void RenderGraph::DeleteImage(ImageProxyId imageId)
	{
		imageProxies.Release(imageId);
	}

	void RenderGraph::SetImageProxyDebugName(ImageProxyId imageId, std::string debugName)
	{
		imageProxies.Get(imageId).imageKey.debugName = debugName;
	}

	void RenderGraph::SetImageViewProxyDebugName(ImageViewProxyId imageViewId, std::string debugName)
	{
		imageViewProxies.Get(imageViewId).debugName = debugName;
	}

	void RenderGraph::DeleteImageView(ImageViewProxyId imageViewId)
	{
		imageViewProxies.Release(imageViewId);
	}

	void RenderGraph::DeleteBuffer(BufferProxyId bufferId)
	{
		bufferProxies.Release(bufferId);
	}

	glm::uvec2 RenderGraph::GetMipSize(ImageProxyId imageProxyId, uint32_t mipLevel)
	{
		auto& imageProxy = imageProxies.Get(imageProxyId);
		switch (imageProxy.type)
		{
		case ImageProxy::Types::External:
			{
				return imageProxy.externalImage->GetMipSize(mipLevel);
			}
			break;
		case ImageProxy::Types::Transient:
			{
				glm::u32 mipMult = (1 << mipLevel);
				return glm::uvec2(imageProxy.imageKey.size.x / mipMult, imageProxy.imageKey.size.y / mipMult);
			}
			break;
		}
		return glm::uvec2(-1, -1);
	}

	glm::uvec2 RenderGraph::GetMipSize(ImageViewProxyId imageViewProxyId, uint32_t mipOffset)
	{
		auto& imageViewProxy = imageViewProxies.Get(imageViewProxyId);
		switch (imageViewProxy.type)
		{
		case ImageViewProxy::Types::External:
			{
				uint32_t mipLevel = imageViewProxy.externalView->GetBaseMipLevel() + mipOffset;
				return imageViewProxy.externalView->GetImageData()->GetMipSize(mipLevel);
			}
			break;
		case ImageViewProxy::Types::Transient:
			{
				uint32_t mipLevel = imageViewProxy.subresourceRange.baseMipLevel + mipOffset;
				return GetMipSize(imageViewProxy.imageProxyId, mipLevel);
			}
			break;
		}
		return glm::uvec2(-1, -1);
	}

	RenderGraph::BufferProxyUnique RenderGraph::AddExternalBuffer(lz::Buffer* buffer)
	{
		BufferProxy bufferProxy;
		bufferProxy.type = BufferProxy::Types::External;
		bufferProxy.bufferKey.elementSize = -1;
		bufferProxy.bufferKey.elementsCount = -1;
		bufferProxy.externalBuffer = buffer;
		return BufferProxyUnique(BufferHandleInfo(this, bufferProxies.Add(std::move(bufferProxy))));
	}

	lz::ImageView* RenderGraph::PassContext::GetImageView(ImageViewProxyId imageViewProxyId)
	{
		return resolvedImageViews[imageViewProxyId.asInt];
	}

	lz::Buffer* RenderGraph::PassContext::GetBuffer(BufferProxyId bufferProxy)
	{
		return resolvedBuffers[bufferProxy.asInt];
	}

	vk::CommandBuffer RenderGraph::PassContext::GetCommandBuffer()
	{
		return commandBuffer;
	}

	lz::RenderPass* RenderGraph::RenderPassContext::GetRenderPass()
	{
		return renderPass;
	}

	RenderGraph::RenderPassDesc::RenderPassDesc()
	{
		profilerTaskName = "RenderPass";
		profilerTaskColor = glm::packUnorm4x8(glm::vec4(1.0f, 0.5f, 0.0f, 1.0f));
	}

	RenderGraph::RenderPassDesc& RenderGraph::RenderPassDesc::SetColorAttachments(
		const std::vector<ImageViewProxyId>& colorAttachmentViewProxies, vk::AttachmentLoadOp loadop,
		vk::ClearValue clearValue)
	{
		this->colorAttachments.resize(colorAttachmentViewProxies.size());
		for (size_t index = 0; index < colorAttachmentViewProxies.size(); ++index)
		{
			this->colorAttachments[index] = {colorAttachmentViewProxies[index], loadop, clearValue};
		}
		return *this;
	}

	RenderGraph::RenderPassDesc& RenderGraph::RenderPassDesc::SetColorAttachments(
		std::vector<Attachment>&& _colorAttachments)
	{
		this->colorAttachments = std::move(_colorAttachments);
		return *this;
	}

	RenderGraph::RenderPassDesc& RenderGraph::RenderPassDesc::SetDepthAttachment(
		ImageViewProxyId _depthAttachmentViewProxyId, vk::AttachmentLoadOp _loadOp, vk::ClearValue _clearValue)
	{
		this->depthAttachment.imageViewProxyId = _depthAttachmentViewProxyId;
		this->depthAttachment.loadOp = _loadOp;
		this->depthAttachment.clearValue = _clearValue;
		return *this;
	}

	RenderGraph::RenderPassDesc& RenderGraph::RenderPassDesc::SetDepthAttachment(Attachment _depthAttachment)
	{
		this->depthAttachment = _depthAttachment;
		return *this;
	}

	RenderGraph::RenderPassDesc& RenderGraph::RenderPassDesc::SetVertexBuffers(
		std::vector<BufferProxyId>&& _vertexBufferProxies)
	{
		this->vertexBufferProxies = std::move(_vertexBufferProxies);
		return *this;
	}

	RenderGraph::RenderPassDesc& RenderGraph::RenderPassDesc::SetInputImages(
		std::vector<ImageViewProxyId>&& _inputImageViewProxies)
	{
		this->inputImageViewProxies = std::move(_inputImageViewProxies);
		return *this;
	}

	RenderGraph::RenderPassDesc& RenderGraph::RenderPassDesc::SetStorageBuffers(
		std::vector<BufferProxyId>&& _inoutStorageBufferProxies)
	{
		this->inoutStorageBufferProxies = _inoutStorageBufferProxies;
		return *this;
	}

	RenderGraph::RenderPassDesc& RenderGraph::RenderPassDesc::SetStorageImages(
		std::vector<ImageViewProxyId>&& _inoutStorageImageProxies)
	{
		this->inoutStorageImageProxies = _inoutStorageImageProxies;
		return *this;
	}

	RenderGraph::RenderPassDesc& RenderGraph::RenderPassDesc::SetRenderAreaExtent(vk::Extent2D _renderAreaExtent)
	{
		this->renderAreaExtent = _renderAreaExtent;
		return *this;
	}

	RenderGraph::RenderPassDesc& RenderGraph::RenderPassDesc::SetRecordFunc(
		std::function<void(RenderPassContext)> _recordFunc)
	{
		this->recordFunc = _recordFunc;
		return *this;
	}

	RenderGraph::RenderPassDesc& RenderGraph::RenderPassDesc::SetProfilerInfo(uint32_t taskColor, std::string taskName)
	{
		this->profilerTaskColor = taskColor;
		this->profilerTaskName = taskName;
		return *this;
	}

	void RenderGraph::AddRenderPass(std::vector<ImageViewProxyId> colorAttachmentImageProxies,
		ImageViewProxyId depthAttachmentImageProxy, std::vector<ImageViewProxyId> inputImageViewProxies,
		vk::Extent2D renderAreaExtent, vk::AttachmentLoadOp loadOp, std::function<void(RenderPassContext)> recordFunc)
	{
		RenderPassDesc renderPassDesc;
		for (const auto& proxy : colorAttachmentImageProxies)
		{
			RenderPassDesc::Attachment colorAttachment = {
				proxy, loadOp, vk::ClearColorValue(std::array<float, 4>{0.03f, 0.03f, 0.03f, 1.0f})
			};
			renderPassDesc.colorAttachments.push_back(colorAttachment);
		}
		renderPassDesc.depthAttachment = {depthAttachmentImageProxy, loadOp, vk::ClearDepthStencilValue(1.0f, 0)};
		renderPassDesc.inputImageViewProxies = inputImageViewProxies;
		renderPassDesc.inoutStorageBufferProxies = {};
		renderPassDesc.renderAreaExtent = renderAreaExtent;
		renderPassDesc.recordFunc = recordFunc;

		AddPass(renderPassDesc);
	}

	void RenderGraph::AddPass(RenderPassDesc& renderPassDesc)
	{
		Task task;
		task.type = Task::Types::RenderPass;
		task.index = renderPassDescs.size();
		AddTask(task);

		renderPassDescs.emplace_back(renderPassDesc);
	}

	void RenderGraph::Clear()
	{
		*this = RenderGraph(physicalDevice, logicalDevice, loader);
	}

	RenderGraph::ComputePassDesc::ComputePassDesc()
	{
		profilerTaskName = "ComputePass";
		profilerTaskColor = lz::Colors::belizeHole;
	}

	RenderGraph::ComputePassDesc& RenderGraph::ComputePassDesc::SetInputImages(
		std::vector<ImageViewProxyId>&& _inputImageViewProxies)
	{
		this->inputImageViewProxies = std::move(_inputImageViewProxies);
		return *this;
	}

	RenderGraph::ComputePassDesc& RenderGraph::ComputePassDesc::SetStorageBuffers(
		std::vector<BufferProxyId>&& _inoutStorageBufferProxies)
	{
		this->inoutStorageBufferProxies = _inoutStorageBufferProxies;
		return *this;
	}

	RenderGraph::ComputePassDesc& RenderGraph::ComputePassDesc::SetStorageImages(
		std::vector<ImageViewProxyId>&& _inoutStorageImageProxies)
	{
		this->inoutStorageImageProxies = _inoutStorageImageProxies;
		return *this;
	}

	RenderGraph::ComputePassDesc& RenderGraph::ComputePassDesc::SetRecordFunc(
		std::function<void(PassContext)> _recordFunc)
	{
		this->recordFunc = _recordFunc;
		return *this;
	}

	RenderGraph::ComputePassDesc& RenderGraph::ComputePassDesc::SetProfilerInfo(uint32_t taskColor,
		std::string taskName)
	{
		this->profilerTaskColor = taskColor;
		this->profilerTaskName = taskName;
		return *this;
	}

	void RenderGraph::AddComputePass(std::vector<BufferProxyId> inoutBufferProxies,
		std::vector<ImageViewProxyId> inputImageViewProxies, std::function<void(PassContext)> recordFunc)
	{
		ComputePassDesc computePassDesc;
		computePassDesc.inoutStorageBufferProxies = inoutBufferProxies;
		computePassDesc.inputImageViewProxies = inputImageViewProxies;
		computePassDesc.recordFunc = recordFunc;

		AddPass(computePassDesc);
	}

	void RenderGraph::AddPass(ComputePassDesc& computePassDesc)
	{
		Task task;
		task.type = Task::Types::ComputePass;
		task.index = computePassDescs.size();
		AddTask(task);

		computePassDescs.emplace_back(computePassDesc);
	}

	RenderGraph::TransferPassDesc::TransferPassDesc()
	{
		profilerTaskName = "TransferPass";
		profilerTaskColor = lz::Colors::silver;
	}

	RenderGraph::TransferPassDesc& RenderGraph::TransferPassDesc::SetSrcImages(
		std::vector<ImageViewProxyId>&& _srcImageViewProxies)
	{
		this->srcImageViewProxies = std::move(_srcImageViewProxies);
		return *this;
	}

	RenderGraph::TransferPassDesc& RenderGraph::TransferPassDesc::SetDstImages(
		std::vector<ImageViewProxyId>&& _dstImageViewProxies)
	{
		this->dstImageViewProxies = std::move(_dstImageViewProxies);
		return *this;
	}

	RenderGraph::TransferPassDesc& RenderGraph::TransferPassDesc::SetSrcBuffers(
		std::vector<BufferProxyId>&& _srcBufferProxies)
	{
		this->srcBufferProxies = std::move(_srcBufferProxies);
		return *this;
	}

	RenderGraph::TransferPassDesc& RenderGraph::TransferPassDesc::SetDstBuffers(
		std::vector<BufferProxyId>&& _dstBufferProxies)
	{
		this->dstBufferProxies = std::move(_dstBufferProxies);
		return *this;
	}

	RenderGraph::TransferPassDesc& RenderGraph::TransferPassDesc::SetRecordFunc(
		std::function<void(PassContext)> _recordFunc)
	{
		this->recordFunc = _recordFunc;
		return *this;
	}

	RenderGraph::TransferPassDesc& RenderGraph::TransferPassDesc::SetProfilerInfo(uint32_t taskColor,
		std::string taskName)
	{
		this->profilerTaskColor = taskColor;
		this->profilerTaskName = taskName;
		return *this;
	}

	void RenderGraph::AddPass(TransferPassDesc& transferPassDesc)
	{
		Task task;
		task.type = Task::Types::TransferPass;
		task.index = transferPassDescs.size();
		AddTask(task);

		transferPassDescs.emplace_back(transferPassDesc);
	}

	RenderGraph::ImagePresentPassDesc& RenderGraph::ImagePresentPassDesc::SetImage(
		ImageViewProxyId _presentImageViewProxyId)
	{
		this->presentImageViewProxyId = _presentImageViewProxyId;
		return *this;
	}

	void RenderGraph::AddPass(ImagePresentPassDesc&& imagePresentDesc)
	{
		Task task;
		task.type = Task::Types::ImagePresent;
		task.index = imagePresentDescs.size();
		AddTask(task);

		imagePresentDescs.push_back(imagePresentDesc);
	}

	void RenderGraph::AddImagePresent(ImageViewProxyId presentImageViewProxyId)
	{
		ImagePresentPassDesc imagePresentDesc;
		imagePresentDesc.presentImageViewProxyId = presentImageViewProxyId;

		AddPass(std::move(imagePresentDesc));
	}

	void RenderGraph::AddPass(FrameSyncBeginPassDesc&& frameSyncBeginDesc)
	{
		Task task;
		task.type = Task::Types::FrameSyncBegin;
		task.index = frameSyncBeginDescs.size();
		AddTask(task);

		frameSyncBeginDescs.push_back(frameSyncBeginDesc);
	}

	void RenderGraph::AddPass(FrameSyncEndPassDesc&& frameSyncEndDesc)
	{
		Task task;
		task.type = Task::Types::FrameSyncEnd;
		task.index = frameSyncEndDescs.size();
		AddTask(task);

		frameSyncEndDescs.push_back(frameSyncEndDesc);
	}

	void RenderGraph::Execute(vk::CommandBuffer commandBuffer, lz::CpuProfiler* cpuProfiler,
		lz::GpuProfiler* gpuProfiler)
	{
		ResolveImages();
		ResolveImageViews();
		ResolveBuffers();

		for (size_t taskIndex = 0; taskIndex < tasks.size(); ++taskIndex)
		{
			auto& task = tasks[taskIndex];
			switch (task.type)
			{
			case Task::Types::RenderPass:
				{
					auto& renderPassDesc = renderPassDescs[task.index];
					auto profilerTask = CreateProfilerTask(renderPassDesc);
					auto gpuTask = gpuProfiler->StartScopedTask(profilerTask.name, profilerTask.color,
					                                            vk::PipelineStageFlagBits::eBottomOfPipe);
					auto cpuTask = cpuProfiler->StartScopedTask(profilerTask.name, profilerTask.color);

					RenderPassContext passContext;
					passContext.resolvedImageViews.resize(imageViewProxies.GetSize(), nullptr);
					passContext.resolvedBuffers.resize(bufferProxies.GetSize(), nullptr);

					for (auto& inputImageViewProxy : renderPassDesc.inputImageViewProxies)
					{
						passContext.resolvedImageViews[inputImageViewProxy.asInt] = GetResolvedImageView(
							taskIndex, inputImageViewProxy);
					}

					for (auto& inoutStorageImageProxy : renderPassDesc.inoutStorageImageProxies)
					{
						passContext.resolvedImageViews[inoutStorageImageProxy.asInt] = GetResolvedImageView(
							taskIndex, inoutStorageImageProxy);
					}

					for (auto& inoutBufferProxy : renderPassDesc.inoutStorageBufferProxies)
					{
						passContext.resolvedBuffers[inoutBufferProxy.asInt] = GetResolvedBuffer(
							taskIndex, inoutBufferProxy);
					}

					for (auto& vertexBufferProxy : renderPassDesc.vertexBufferProxies)
					{
						passContext.resolvedBuffers[vertexBufferProxy.asInt] = GetResolvedBuffer(
							taskIndex, vertexBufferProxy);
					}

					vk::PipelineStageFlags srcStage;
					vk::PipelineStageFlags dstStage;
					std::vector<vk::ImageMemoryBarrier> imageBarriers;

					for (auto inputImageViewProxy : renderPassDesc.inputImageViewProxies)
					{
						auto imageView = GetResolvedImageView(taskIndex, inputImageViewProxy);
						AddImageTransitionBarriers(imageView, ImageUsageTypes::GraphicsShaderRead, taskIndex,
						                           srcStage, dstStage, imageBarriers);
					}

					for (auto& inoutStorageImageProxy : renderPassDesc.inoutStorageImageProxies)
					{
						auto imageView = GetResolvedImageView(taskIndex, inoutStorageImageProxy);
						AddImageTransitionBarriers(imageView, ImageUsageTypes::GraphicsShaderReadWrite, taskIndex,
						                           srcStage, dstStage, imageBarriers);
					}

					for (auto& colorAttachment : renderPassDesc.colorAttachments)
					{
						auto imageView = GetResolvedImageView(taskIndex, colorAttachment.imageViewProxyId);
						AddImageTransitionBarriers(imageView, ImageUsageTypes::ColorAttachment, taskIndex, srcStage,
						                           dstStage, imageBarriers);
					}

					if (!(renderPassDesc.depthAttachment.imageViewProxyId == ImageViewProxyId()))
					{
						auto imageView = GetResolvedImageView(
							taskIndex, renderPassDesc.depthAttachment.imageViewProxyId);
						AddImageTransitionBarriers(imageView, ImageUsageTypes::DepthAttachment, taskIndex, srcStage,
						                           dstStage, imageBarriers);
					}

					std::vector<vk::BufferMemoryBarrier> bufferBarriers;

					for (auto vertexBufferProxy : renderPassDesc.vertexBufferProxies)
					{
						auto storageBuffer = GetResolvedBuffer(taskIndex, vertexBufferProxy);
						AddBufferBarriers(storageBuffer, BufferUsageTypes::VertexBuffer, taskIndex, srcStage,
						                  dstStage, bufferBarriers);
					}

					for (auto inoutBufferProxy : renderPassDesc.inoutStorageBufferProxies)
					{
						auto storageBuffer = GetResolvedBuffer(taskIndex, inoutBufferProxy);
						AddBufferBarriers(storageBuffer, BufferUsageTypes::GraphicsShaderReadWrite, taskIndex,
						                  srcStage, dstStage, bufferBarriers);
					}

					if (imageBarriers.size() > 0 || bufferBarriers.size() > 0)
					{
						commandBuffer.pipelineBarrier(srcStage, dstStage, vk::DependencyFlags(), {}, bufferBarriers,
						                              imageBarriers);
					}

					std::vector<FramebufferCache::Attachment> colorAttachments;
					FramebufferCache::Attachment depthAttachment;

					lz::RenderPassCache::RenderPassKey renderPassKey;

					for (auto& attachment : renderPassDesc.colorAttachments)
					{
						auto imageView = GetResolvedImageView(taskIndex, attachment.imageViewProxyId);

						renderPassKey.colorAttachmentDescs.push_back({
							imageView->GetImageData()->GetFormat(), attachment.loadOp, attachment.clearValue
						});
						colorAttachments.push_back({imageView, attachment.clearValue});
					}
					bool depthPresent = !(renderPassDesc.depthAttachment.imageViewProxyId == ImageViewProxyId());
					if (depthPresent)
					{
						auto imageView = GetResolvedImageView(
							taskIndex, renderPassDesc.depthAttachment.imageViewProxyId);

						renderPassKey.depthAttachmentDesc = {
							imageView->GetImageData()->GetFormat(), renderPassDesc.depthAttachment.loadOp,
							renderPassDesc.depthAttachment.clearValue
						};
						depthAttachment = {imageView, renderPassDesc.depthAttachment.clearValue};
					}
					else
					{
						renderPassKey.depthAttachmentDesc.format = vk::Format::eUndefined;
					}

					auto renderPass = renderPassCache.GetRenderPass(renderPassKey);
					passContext.renderPass = renderPass;

					framebufferCache.BeginPass(commandBuffer, colorAttachments,
					                           depthPresent ? (&depthAttachment) : nullptr, renderPass,
					                           renderPassDesc.renderAreaExtent);
					passContext.commandBuffer = commandBuffer;
					renderPassDesc.recordFunc(passContext);
					framebufferCache.EndPass(commandBuffer);
				}
				break;
			case Task::Types::ComputePass:
				{
					auto& computePassDesc = computePassDescs[task.index];
					auto profilerTask = CreateProfilerTask(computePassDesc);
					auto gpuTask = gpuProfiler->StartScopedTask(profilerTask.name, profilerTask.color,
					                                            vk::PipelineStageFlagBits::eBottomOfPipe);
					auto cpuTask = cpuProfiler->StartScopedTask(profilerTask.name, profilerTask.color);

					PassContext passContext;
					passContext.resolvedImageViews.resize(imageViewProxies.GetSize(), nullptr);
					passContext.resolvedBuffers.resize(bufferProxies.GetSize(), nullptr);

					for (auto& inputImageViewProxy : computePassDesc.inputImageViewProxies)
					{
						passContext.resolvedImageViews[inputImageViewProxy.asInt] = GetResolvedImageView(
							taskIndex, inputImageViewProxy);
					}

					for (auto& inoutBufferProxy : computePassDesc.inoutStorageBufferProxies)
					{
						passContext.resolvedBuffers[inoutBufferProxy.asInt] = GetResolvedBuffer(
							taskIndex, inoutBufferProxy);
					}

					for (auto& inoutStorageImageProxy : computePassDesc.inoutStorageImageProxies)
					{
						passContext.resolvedImageViews[inoutStorageImageProxy.asInt] = GetResolvedImageView(
							taskIndex, inoutStorageImageProxy);
					}

					vk::PipelineStageFlags srcStage;
					vk::PipelineStageFlags dstStage;

					std::vector<vk::ImageMemoryBarrier> imageBarriers;
					for (auto inputImageViewProxy : computePassDesc.inputImageViewProxies)
					{
						auto imageView = GetResolvedImageView(taskIndex, inputImageViewProxy);
						AddImageTransitionBarriers(imageView, ImageUsageTypes::ComputeShaderRead, taskIndex,
						                           srcStage, dstStage, imageBarriers);
					}

					for (auto& inoutStorageImageProxy : computePassDesc.inoutStorageImageProxies)
					{
						auto imageView = GetResolvedImageView(taskIndex, inoutStorageImageProxy);
						AddImageTransitionBarriers(imageView, ImageUsageTypes::ComputeShaderReadWrite, taskIndex,
						                           srcStage, dstStage, imageBarriers);
					}

					std::vector<vk::BufferMemoryBarrier> bufferBarriers;
					for (auto inoutBufferProxy : computePassDesc.inoutStorageBufferProxies)
					{
						auto storageBuffer = GetResolvedBuffer(taskIndex, inoutBufferProxy);
						AddBufferBarriers(storageBuffer, BufferUsageTypes::ComputeShaderReadWrite, taskIndex,
						                  srcStage, dstStage, bufferBarriers);
					}

					if (imageBarriers.size() > 0 || bufferBarriers.size() > 0)
					{
						commandBuffer.pipelineBarrier(srcStage, dstStage, vk::DependencyFlags(), {}, bufferBarriers,
						                              imageBarriers);
					}

					passContext.commandBuffer = commandBuffer;
					if (computePassDesc.recordFunc)
					{
						computePassDesc.recordFunc(passContext);
					}
				}
				break;
			case Task::Types::TransferPass:
				{
					auto& transferPassDesc = transferPassDescs[task.index];
					auto profilerTask = CreateProfilerTask(transferPassDesc);
					auto gpuTask = gpuProfiler->StartScopedTask(profilerTask.name, profilerTask.color,
					                                            vk::PipelineStageFlagBits::eBottomOfPipe);
					auto cpuTask = cpuProfiler->StartScopedTask(profilerTask.name, profilerTask.color);

					PassContext passContext;
					passContext.resolvedImageViews.resize(imageViewProxies.GetSize(), nullptr);
					passContext.resolvedBuffers.resize(bufferProxies.GetSize(), nullptr);

					for (auto& srcImageViewProxy : transferPassDesc.srcImageViewProxies)
					{
						passContext.resolvedImageViews[srcImageViewProxy.asInt] = GetResolvedImageView(
							taskIndex, srcImageViewProxy);
					}
					for (auto& dstImageViewProxy : transferPassDesc.dstImageViewProxies)
					{
						passContext.resolvedImageViews[dstImageViewProxy.asInt] = GetResolvedImageView(
							taskIndex, dstImageViewProxy);
					}

					for (auto& srcBufferProxy : transferPassDesc.srcBufferProxies)
					{
						passContext.resolvedBuffers[srcBufferProxy.asInt] = GetResolvedBuffer(
							taskIndex, srcBufferProxy);
					}

					for (auto& dstBufferProxy : transferPassDesc.dstBufferProxies)
					{
						passContext.resolvedBuffers[dstBufferProxy.asInt] = GetResolvedBuffer(
							taskIndex, dstBufferProxy);
					}

					vk::PipelineStageFlags srcStage;
					vk::PipelineStageFlags dstStage;

					std::vector<vk::ImageMemoryBarrier> imageBarriers;
					for (auto srcImageViewProxy : transferPassDesc.srcImageViewProxies)
					{
						auto imageView = GetResolvedImageView(taskIndex, srcImageViewProxy);
						AddImageTransitionBarriers(imageView, ImageUsageTypes::TransferSrc, taskIndex, srcStage,
						                           dstStage, imageBarriers);
					}

					for (auto dstImageViewProxy : transferPassDesc.dstImageViewProxies)
					{
						auto imageView = GetResolvedImageView(taskIndex, dstImageViewProxy);
						AddImageTransitionBarriers(imageView, ImageUsageTypes::TransferDst, taskIndex, srcStage,
						                           dstStage, imageBarriers);
					}

					std::vector<vk::BufferMemoryBarrier> bufferBarriers;
					for (auto srcBufferProxy : transferPassDesc.srcBufferProxies)
					{
						auto storageBuffer = GetResolvedBuffer(taskIndex, srcBufferProxy);
						AddBufferBarriers(storageBuffer, BufferUsageTypes::TransferSrc, taskIndex, srcStage,
						                  dstStage, bufferBarriers);
					}

					for (auto dstBufferProxy : transferPassDesc.dstBufferProxies)
					{
						auto storageBuffer = GetResolvedBuffer(taskIndex, dstBufferProxy);
						AddBufferBarriers(storageBuffer, BufferUsageTypes::TransferSrc, taskIndex, srcStage,
						                  dstStage, bufferBarriers);
					}

					if (imageBarriers.size() > 0 || bufferBarriers.size() > 0)
						commandBuffer.pipelineBarrier(srcStage, dstStage, vk::DependencyFlags(), {}, bufferBarriers,
						                              imageBarriers);

					passContext.commandBuffer = commandBuffer;
					if (transferPassDesc.recordFunc)
						transferPassDesc.recordFunc(passContext);
				}
				break;
			case Task::Types::ImagePresent:
				{
					auto imagePesentDesc = imagePresentDescs[task.index];
					auto profilerTask = CreateProfilerTask(imagePesentDesc);
					auto gpuTask = gpuProfiler->StartScopedTask(profilerTask.name, profilerTask.color,
					                                            vk::PipelineStageFlagBits::eBottomOfPipe);
					auto cpuTask = cpuProfiler->StartScopedTask(profilerTask.name, profilerTask.color);

					vk::PipelineStageFlags srcStage;
					vk::PipelineStageFlags dstStage;
					std::vector<vk::ImageMemoryBarrier> imageBarriers;
					{
						auto imageView = GetResolvedImageView(taskIndex, imagePesentDesc.presentImageViewProxyId);
						AddImageTransitionBarriers(imageView, ImageUsageTypes::Present, taskIndex, srcStage,
						                           dstStage, imageBarriers);
					}

					if (imageBarriers.size() > 0)
						commandBuffer.pipelineBarrier(srcStage, dstStage, vk::DependencyFlags(), {}, {},
						                              imageBarriers);
				}
				break;
			case Task::Types::FrameSyncBegin:
				{
					auto frameSyncDesc = frameSyncBeginDescs[task.index];
					auto profilerTask = CreateProfilerTask(frameSyncDesc);
					auto gpuTask = gpuProfiler->StartScopedTask(profilerTask.name, profilerTask.color,
					                                            vk::PipelineStageFlagBits::eBottomOfPipe);
					auto cpuTask = cpuProfiler->StartScopedTask(profilerTask.name, profilerTask.color);

					std::vector<vk::ImageMemoryBarrier> imageBarriers;
					vk::PipelineStageFlags srcStage = vk::PipelineStageFlagBits::eBottomOfPipe;
					vk::PipelineStageFlags dstStage = vk::PipelineStageFlagBits::eTopOfPipe;

					auto memoryBarrier = vk::MemoryBarrier();
					commandBuffer.pipelineBarrier(srcStage, dstStage, vk::DependencyFlags(), {memoryBarrier}, {},
					                              {});
				}
				break;
			case Task::Types::FrameSyncEnd:
				{
					auto frameSyncDesc = frameSyncEndDescs[task.index];
					auto profilerTask = CreateProfilerTask(frameSyncDesc);
					auto gpuTask = gpuProfiler->StartScopedTask(profilerTask.name, profilerTask.color,
					                                            vk::PipelineStageFlagBits::eBottomOfPipe);
					auto cpuTask = cpuProfiler->StartScopedTask(profilerTask.name, profilerTask.color);

					std::vector<vk::ImageMemoryBarrier> imageBarriers;
					vk::PipelineStageFlags srcStage = vk::PipelineStageFlagBits::eBottomOfPipe;
					vk::PipelineStageFlags dstStage = vk::PipelineStageFlagBits::eTopOfPipe;
					for (auto imageViewProxy : imageViewProxies)
					{
						if (imageViewProxy.externalView != nullptr && imageViewProxy.externalUsageType !=
							lz::ImageUsageTypes::Unknown && imageViewProxy.externalUsageType !=
							lz::ImageUsageTypes::None)
							AddImageTransitionBarriers(imageViewProxy.externalView,
							                           imageViewProxy.externalUsageType, taskIndex, srcStage,
							                           dstStage, imageBarriers);
					}

					/*std::vector<vk::BufferMemoryBarrier> bufferBarriers;
						for (auto bufferProxy : bufferProxies)
						{
						  if(bufferProxy.externalBuffer != nullptr)
						  auto storageBuffer = GetResolvedBuffer(taskIndex, inoutBufferProxy);
						  AddBufferBarriers(storageBuffer, BufferUsageTypes::ComputeShaderReadWrite, taskIndex, srcStage, dstStage, bufferBarriers);
						}*/

					if (imageBarriers.size() > 0)
						commandBuffer.pipelineBarrier(srcStage, dstStage, vk::DependencyFlags(), {}, {},
						                              imageBarriers);
				}
				break;
			}
		}

		FlushExternalImages(commandBuffer, cpuProfiler, gpuProfiler);

		renderPassDescs.clear();
		transferPassDescs.clear();
		imagePresentDescs.clear();
		frameSyncBeginDescs.clear();
		frameSyncEndDescs.clear();
		tasks.clear();
	}

	void RenderGraph::FlushExternalImages(vk::CommandBuffer commandBuffer, lz::CpuProfiler* cpuProfiler,
		lz::GpuProfiler* gpuProfiler)
	{
		//need to make sure I don't override the same mip level of an image twice if there's more than view for it

		/*if (tasks.size() == 0) return;
			size_t lastTaskIndex = tasks.size() - 1;

			for (auto &imageViewProxy : this->imageViewProxies)
			{
			  if (imageViewProxy.type == ImageViewProxy::Types::External)
			  {
				vk::PipelineStageFlags srcStage;
				vk::PipelineStageFlags dstStage;

				std::vector<vk::ImageMemoryBarrier> imageBarriers;
				if (imageViewProxy.externalUsageType != lz::ImageUsageTypes::Unknown)
				{
				  AddImageTransitionBarriers(imageViewProxy.externalView, imageViewProxy.externalUsageType, tasks.size(), srcStage, dstStage, imageBarriers);
				}

				if (imageBarriers.size() > 0)
				  commandBuffer.pipelineBarrier(srcStage, dstStage, vk::DependencyFlags(), {}, {}, imageBarriers);
			  }
			}*/
	}

	bool RenderGraph::ImageViewContainsSubresource(lz::ImageView* imageView, lz::ImageData* imageData,
		uint32_t mipLevel, uint32_t arrayLayer)
	{
		return (
			imageView->GetImageData() == imageData &&
			arrayLayer >= imageView->GetBaseArrayLayer() && arrayLayer < imageView->GetBaseArrayLayer() + imageView
			->GetArrayLayersCount() &&
			mipLevel >= imageView->GetBaseMipLevel() && mipLevel < imageView->GetBaseMipLevel() + imageView->
			GetMipLevelsCount());
	}

	ImageUsageTypes RenderGraph::GetTaskImageSubresourceUsageType(size_t taskIndex, lz::ImageData* imageData,
		uint32_t mipLevel, uint32_t arrayLayer)
	{
		Task& task = tasks[taskIndex];
		switch (task.type)
		{
		case Task::Types::RenderPass:
			{
				auto& renderPassDesc = renderPassDescs[task.index];
				for (const auto& colorAttachment : renderPassDesc.colorAttachments)
				{
					auto attachmentImageView = GetResolvedImageView(taskIndex, colorAttachment.imageViewProxyId);
					if (ImageViewContainsSubresource(attachmentImageView, imageData, mipLevel, arrayLayer))
					{
						return ImageUsageTypes::ColorAttachment;
					}
				}

				if (!(renderPassDesc.depthAttachment.imageViewProxyId == ImageViewProxyId()))
				{
					auto attachmentImageView = GetResolvedImageView(
						taskIndex, renderPassDesc.depthAttachment.imageViewProxyId);
					if (ImageViewContainsSubresource(attachmentImageView, imageData, mipLevel, arrayLayer))
					{
						return ImageUsageTypes::DepthAttachment;
					}
				}

				for (const auto& imageViewProxy : renderPassDesc.inputImageViewProxies)
				{
					if (ImageViewContainsSubresource(GetResolvedImageView(taskIndex, imageViewProxy), imageData,
					                                 mipLevel, arrayLayer))
						return ImageUsageTypes::GraphicsShaderRead;
				}

				for (const auto& imageViewProxy : renderPassDesc.inoutStorageImageProxies)
				{
					if (ImageViewContainsSubresource(GetResolvedImageView(taskIndex, imageViewProxy), imageData,
					                                 mipLevel, arrayLayer))
						return ImageUsageTypes::GraphicsShaderReadWrite;
				}
			}
			break;
		case Task::Types::ComputePass:
			{
				auto& computePassDesc = computePassDescs[task.index];
				for (auto imageViewProxy : computePassDesc.inputImageViewProxies)
				{
					if (ImageViewContainsSubresource(GetResolvedImageView(taskIndex, imageViewProxy), imageData,
					                                 mipLevel, arrayLayer))
						return ImageUsageTypes::ComputeShaderRead;
				}
				for (auto imageViewProxy : computePassDesc.inoutStorageImageProxies)
				{
					if (ImageViewContainsSubresource(GetResolvedImageView(taskIndex, imageViewProxy), imageData,
					                                 mipLevel, arrayLayer))
						return ImageUsageTypes::ComputeShaderReadWrite;
				}
			}
			break;
		case Task::Types::TransferPass:
			{
				auto& transferPassDesc = transferPassDescs[task.index];
				for (auto srcImageViewProxy : transferPassDesc.srcImageViewProxies)
				{
					if (ImageViewContainsSubresource(GetResolvedImageView(taskIndex, srcImageViewProxy), imageData,
					                                 mipLevel, arrayLayer))
						return ImageUsageTypes::TransferSrc;
				}
				for (auto dstImageViewProxy : transferPassDesc.dstImageViewProxies)
				{
					if (ImageViewContainsSubresource(GetResolvedImageView(taskIndex, dstImageViewProxy), imageData,
					                                 mipLevel, arrayLayer))
						return ImageUsageTypes::TransferDst;
				}
			}
			break;
		case Task::Types::ImagePresent:
			{
				auto& imagePresentDesc = imagePresentDescs[task.index];
				if (ImageViewContainsSubresource(
					GetResolvedImageView(taskIndex, imagePresentDesc.presentImageViewProxyId), imageData, mipLevel,
					arrayLayer))
					return ImageUsageTypes::Present;
			}
			break;
		}

		return ImageUsageTypes::None;
	}

	BufferUsageTypes RenderGraph::GetTaskBufferUsageType(size_t taskIndex, lz::Buffer* buffer)
	{
		Task& task = tasks[taskIndex];

		switch (task.type)
		{
		case Task::Types::RenderPass:
			{
				auto& renderPassDesc = renderPassDescs[task.index];
				for (auto storageBufferProxy : renderPassDesc.inoutStorageBufferProxies)
				{
					auto storageBuffer = GetResolvedBuffer(taskIndex, storageBufferProxy);
					if (buffer->get_handle() == storageBuffer->get_handle())
					{
						return BufferUsageTypes::GraphicsShaderReadWrite;
					}
				}

				for (auto vertexBufferProxy : renderPassDesc.vertexBufferProxies)
				{
					auto vertexBuffer = GetResolvedBuffer(taskIndex, vertexBufferProxy);
					if (buffer->get_handle() == vertexBuffer->get_handle())
					{
						return BufferUsageTypes::VertexBuffer;
					}
				}
			}
			break;
		case Task::Types::ComputePass:
			{
				auto& computePassDesc = computePassDescs[task.index];
				for (auto storageBufferProxy : computePassDesc.inoutStorageBufferProxies)
				{
					auto storageBuffer = GetResolvedBuffer(taskIndex, storageBufferProxy);
					if (buffer->get_handle() == storageBuffer->get_handle())
						return BufferUsageTypes::ComputeShaderReadWrite;
				}
			}
			break;

		case Task::Types::TransferPass:
			{
				auto& transferPassDesc = transferPassDescs[task.index];
				for (auto srcBufferProxy : transferPassDesc.srcBufferProxies)
				{
					auto srcBuffer = GetResolvedBuffer(taskIndex, srcBufferProxy);
					if (buffer->get_handle() == srcBuffer->get_handle())
						return BufferUsageTypes::TransferSrc;
				}
				for (auto dstBufferProxy : transferPassDesc.dstBufferProxies)
				{
					auto dstBuffer = GetResolvedBuffer(taskIndex, dstBufferProxy);
					if (buffer->get_handle() == dstBuffer->get_handle())
						return BufferUsageTypes::TransferSrc;
				}
			}
			break;
		}
		return BufferUsageTypes::None;
	}

	ImageUsageTypes RenderGraph::GetLastImageSubresourceUsageType(size_t taskIndex, lz::ImageData* imageData,
		uint32_t mipLevel, uint32_t arrayLayer)
	{
		for (size_t taskOffset = 0; taskOffset < taskIndex; taskOffset++)
		{
			size_t prevTaskIndex = taskIndex - taskOffset - 1;
			auto usageType = GetTaskImageSubresourceUsageType(prevTaskIndex, imageData, mipLevel, arrayLayer);
			if (usageType != ImageUsageTypes::None)
				return usageType;
		}

		for (auto& imageViewProxy : imageViewProxies)
		{
			if (imageViewProxy.type == ImageViewProxy::Types::External && imageViewProxy.externalView->
				GetImageData() == imageData)
			{
				return imageViewProxy.externalUsageType;
			}
		}
		return ImageUsageTypes::None;
	}

	BufferUsageTypes RenderGraph::GetLastBufferUsageType(size_t taskIndex, lz::Buffer* buffer)
	{
		for (size_t taskOffset = 1; taskOffset < taskIndex; taskOffset++)
		{
			size_t prevTaskIndex = taskIndex - taskOffset;
			auto usageType = GetTaskBufferUsageType(prevTaskIndex, buffer);
			if (usageType != BufferUsageTypes::None)
				return usageType;
		}
		return BufferUsageTypes::None;
	}

	void RenderGraph::FlushImageTransitionBarriers(lz::ImageData* imageData, vk::ImageSubresourceRange range,
		ImageUsageTypes srcUsageType, ImageUsageTypes dstUsageType, vk::PipelineStageFlags& srcStage,
		vk::PipelineStageFlags& dstStage, std::vector<vk::ImageMemoryBarrier>& imageBarriers)
	{
		if (IsImageBarrierNeeded(srcUsageType, dstUsageType) && range.layerCount > 0 && range.levelCount > 0)
		{
			auto srcImageAccessPattern = GetSrcImageAccessPattern(srcUsageType);
			auto dstImageAccessPattern = GetDstImageAccessPattern(dstUsageType);
			auto imageBarrier = vk::ImageMemoryBarrier()
			                    .setSrcAccessMask(srcImageAccessPattern.accessMask)
			                    .setDstAccessMask(dstImageAccessPattern.accessMask)
			                    .setOldLayout(srcImageAccessPattern.layout)
			                    .setNewLayout(dstImageAccessPattern.layout)
			                    .setSubresourceRange(range)
			                    .setImage(imageData->GetHandle());

			if (srcImageAccessPattern.queueFamilyType == dstImageAccessPattern.queueFamilyType)
			{
				imageBarrier
					.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
			}
			else
			{
				// TODO: transfer queue
				imageBarrier
					.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
			}

			srcStage |= srcImageAccessPattern.stage;
			dstStage |= dstImageAccessPattern.stage;

			imageBarriers.push_back(imageBarrier);
		}
	}

	void RenderGraph::AddImageTransitionBarriers(lz::ImageView* imageView, ImageUsageTypes dstUsageType,
		size_t dstTaskIndex, vk::PipelineStageFlags& srcStage, vk::PipelineStageFlags& dstStage,
		std::vector<vk::ImageMemoryBarrier>& imageBarriers)
	{
		auto range = vk::ImageSubresourceRange()
			.setAspectMask(imageView->GetImageData()->GetAspectFlags());

		for (uint32_t arrayLayer = imageView->GetBaseArrayLayer(); arrayLayer < imageView->GetBaseArrayLayer() +
		     imageView->GetArrayLayersCount(); ++arrayLayer)
		{
			range.setBaseArrayLayer(arrayLayer)
			     .setLayerCount(1)
			     .setBaseMipLevel(imageView->GetBaseMipLevel())
			     .setLevelCount(0);
			ImageUsageTypes prevSubresourceUsageType = ImageUsageTypes::None;

			for (uint32_t mipLevel = imageView->GetBaseMipLevel(); mipLevel < imageView->GetBaseMipLevel() +
			     imageView->GetMipLevelsCount(); ++mipLevel)
			{
				auto lastUsageType = GetLastImageSubresourceUsageType(
					dstTaskIndex, imageView->GetImageData(), mipLevel, arrayLayer);
				if (prevSubresourceUsageType != lastUsageType)
				{
					FlushImageTransitionBarriers(imageView->GetImageData(),
					                             range, prevSubresourceUsageType, dstUsageType, srcStage, dstStage,
					                             imageBarriers);
					range.setBaseMipLevel(mipLevel)
					     .setLevelCount(0);
					prevSubresourceUsageType = lastUsageType;
				}
				range.levelCount++;
			}
			FlushImageTransitionBarriers(imageView->GetImageData(), range, prevSubresourceUsageType, dstUsageType,
			                             srcStage, dstStage, imageBarriers);
		}
	}

	void RenderGraph::FlushBufferTransitionBarriers(lz::Buffer* buffer, BufferUsageTypes srcUsageType,
		BufferUsageTypes dstUsageType, vk::PipelineStageFlags& srcStage, vk::PipelineStageFlags& dstStage,
		std::vector<vk::BufferMemoryBarrier>& bufferBarriers)
	{
		if (IsBufferBarrierNeeded(srcUsageType, dstUsageType))
		{
			auto srcBufferAccessPattern = GetSrcBufferAccessPattern(srcUsageType);
			auto dstBufferAccessPattern = GetDstBufferAccessPattern(dstUsageType);
			auto bufferBarrier = vk::BufferMemoryBarrier()
			                     .setSrcAccessMask(srcBufferAccessPattern.accessMask)
			                     .setOffset(0)
			                     .setSize(VK_WHOLE_SIZE)
			                     .setDstAccessMask(dstBufferAccessPattern.accessMask)
			                     .setBuffer(buffer->get_handle());

			if (srcBufferAccessPattern.queueFamilyType == dstBufferAccessPattern.queueFamilyType)
			{
				bufferBarrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				             .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
			}
			else
			{
				// TODO: transfer queue 
				bufferBarrier
					.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
					.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
			}

			srcStage |= srcBufferAccessPattern.stage;
			dstStage |= dstBufferAccessPattern.stage;
			bufferBarriers.push_back(bufferBarrier);
		}
	}

	void RenderGraph::AddBufferBarriers(lz::Buffer* buffer, BufferUsageTypes dstUsageType, size_t dstTaskIndex,
		vk::PipelineStageFlags& srcStage, vk::PipelineStageFlags& dstStage,
		std::vector<vk::BufferMemoryBarrier>& bufferBarriers)
	{
		auto lastUsageType = GetLastBufferUsageType(dstTaskIndex, buffer);
		FlushBufferTransitionBarriers(buffer, lastUsageType, dstUsageType, srcStage, dstStage, bufferBarriers);
	}

	bool RenderGraph::ImageViewProxy::Contains(const ImageViewProxy& other)
	{
		if (type == Types::Transient && subresourceRange.Contains(other.subresourceRange) && type == other.type
			&& imageProxyId == other.imageProxyId)
		{
			return true;
		}

		if (type == Types::External && externalView == other.externalView)
		{
			return true;
		}
		return false;
	}

	void RenderGraph::ResolveImages()
	{
		imageCache.Release();

		for (auto& imageProxy : imageProxies)
		{
			switch (imageProxy.type)
			{
			case ImageProxy::Types::External:
				{
					imageProxy.resolvedImage = imageProxy.externalImage;
				}
				break;
			case ImageProxy::Types::Transient:
				{
					imageProxy.resolvedImage = imageCache.GetImage(imageProxy.imageKey);
				}
				break;
			}
		}
	}

	lz::ImageData* RenderGraph::GetResolvedImage(size_t taskIndex, ImageProxyId imageProxy)
	{
		return imageProxies.Get(imageProxy).resolvedImage;
	}

	void RenderGraph::ResolveImageViews()
	{
		for (auto& imageViewProxy : imageViewProxies)
		{
			switch (imageViewProxy.type)
			{
			case ImageViewProxy::Types::External:
				{
					imageViewProxy.resolvedImageView = imageViewProxy.externalView;
				}
				break;
			case ImageViewProxy::Types::Transient:
				{
					ImageViewCache::ImageViewKey imageViewKey;
					imageViewKey.image = GetResolvedImage(0, imageViewProxy.imageProxyId);
					imageViewKey.subresourceRange = imageViewProxy.subresourceRange;
					imageViewKey.debugName = imageViewProxy.debugName + "[img: " + imageProxies.Get(
						imageViewProxy.imageProxyId).imageKey.debugName + "]";

					imageViewProxy.resolvedImageView = imageViewCache.GetImageView(imageViewKey);
				}
				break;
			}
		}
	}

	lz::ImageView* RenderGraph::GetResolvedImageView(size_t taskIndex, ImageViewProxyId imageViewProxyId)
	{
		return imageViewProxies.Get(imageViewProxyId).resolvedImageView;
	}

	void RenderGraph::ResolveBuffers()
	{
		bufferCache.Release();

		for (auto& bufferProxy : bufferProxies)
		{
			switch (bufferProxy.type)
			{
			case BufferProxy::Types::External:
				{
					bufferProxy.resolvedBuffer = bufferProxy.externalBuffer;
				}
				break;
			case BufferProxy::Types::Transient:
				{
					bufferProxy.resolvedBuffer = bufferCache.GetBuffer(bufferProxy.bufferKey);
				}
				break;
			}
		}
	}

	lz::Buffer* RenderGraph::GetResolvedBuffer(size_t taskIndex, BufferProxyId bufferProxyId)
	{
		return bufferProxies.Get(bufferProxyId).resolvedBuffer;
	}

	void RenderGraph::AddTask(Task task)
	{
		tasks.push_back(task);
	}

	lz::ProfilerTask RenderGraph::CreateProfilerTask(const RenderPassDesc& renderPassDesc)
	{
		lz::ProfilerTask task;
		task.startTime = -1.0f;
		task.endTime = -1.0f;
		task.name = renderPassDesc.profilerTaskName;
		task.color = renderPassDesc.profilerTaskColor;
		return task;
	}

	lz::ProfilerTask RenderGraph::CreateProfilerTask(const ComputePassDesc& computePassDesc)
	{
		lz::ProfilerTask task;
		task.startTime = -1.0f;
		task.endTime = -1.0f;
		task.name = computePassDesc.profilerTaskName;
		task.color = computePassDesc.profilerTaskColor;
		return task;
	}

	lz::ProfilerTask RenderGraph::CreateProfilerTask(const TransferPassDesc& transferPassDesc)
	{
		lz::ProfilerTask task;
		task.startTime = -1.0f;
		task.endTime = -1.0f;
		task.name = transferPassDesc.profilerTaskName;
		task.color = transferPassDesc.profilerTaskColor;
		return task;
	}

	lz::ProfilerTask RenderGraph::CreateProfilerTask(const ImagePresentPassDesc& imagePresentPassDesc)
	{
		lz::ProfilerTask task;
		task.startTime = -1.0f;
		task.endTime = -1.0f;
		task.name = "ImagePresent";
		task.color = glm::packUnorm4x8(glm::vec4(0.0f, 1.0f, 0.5f, 1.0f));
		return task;
	}

	lz::ProfilerTask RenderGraph::CreateProfilerTask(const FrameSyncBeginPassDesc& frameSyncBeginPassDesc)
	{
		lz::ProfilerTask task;
		task.startTime = -1.0f;
		task.endTime = -1.0f;
		task.name = "FrameSyncBegin";
		task.color = glm::packUnorm4x8(glm::vec4(0.0f, 0.5f, 1.0f, 1.0f));
		return task;
	}

	lz::ProfilerTask RenderGraph::CreateProfilerTask(const FrameSyncEndPassDesc& frameSyncEndPassDesc)
	{
		lz::ProfilerTask task;
		task.startTime = -1.0f;
		task.endTime = -1.0f;
		task.name = "FrameSyncEnd";
		task.color = glm::packUnorm4x8(glm::vec4(0.0f, 0.5f, 1.0f, 1.0f));
		return task;
	}
}
