#pragma once

#include "RenderPassCache.h"

namespace lz
{
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
			bool operator <(const ImageKey& other) const
			{
				auto lUsageFlags = VkMemoryMapFlags(usageFlags);
				auto rUsageFlags = VkMemoryMapFlags(other.usageFlags);
				return std::tie(format, mipsCount, arrayLayersCount, lUsageFlags, size.x, size.y, size.z) < std::tie(
					other.format, other.mipsCount, other.arrayLayersCount, rUsageFlags, other.size.x, other.size.y,
					other.size.z);
			}

			vk::Format format;
			vk::ImageUsageFlags usageFlags;
			uint32_t mipsCount;
			uint32_t arrayLayersCount;
			glm::uvec3 size;
			std::string debugName;
		};

		ImageCache(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice, vk::DispatchLoaderDynamic loader)
			: physicalDevice(physicalDevice),
			  logicalDevice(logicalDevice),
			  loader(loader)
		{
		}

		void Release()
		{
			for (auto& cacheEntry : imageCache)
			{
				cacheEntry.second.usedCount = 0;
			}
		}

		lz::ImageData* GetImage(ImageKey imageKey)
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
		}

	private:
		struct ImageCacheEntry
		{
			ImageCacheEntry() : usedCount(0)
			{
			}

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

			bool operator <(const ImageViewKey& other) const
			{
				return std::tie(image, subresourceRange) < std::tie(other.image, other.subresourceRange);
			}
		};

		ImageViewCache(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice)
			: physicalDevice(physicalDevice), logicalDevice(logicalDevice)
		{
		}

		lz::ImageView* GetImageView(ImageViewKey imageViewKey)
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

	private:
		std::map<ImageViewKey, std::unique_ptr<lz::ImageView>> imageViewCache;
		vk::PhysicalDevice physicalDevice;
		vk::Device logicalDevice;
	};

	class BufferCache
	{
	public:
		BufferCache(vk::PhysicalDevice _physicalDevice, vk::Device _logicalDevice) : physicalDevice(_physicalDevice),
			logicalDevice(_logicalDevice)
		{
		}

		struct BufferKey
		{
			uint32_t elementSize;
			uint32_t elementsCount;

			bool operator <(const BufferKey& other) const
			{
				return std::tie(elementSize, elementsCount) < std::tie(other.elementSize, other.elementsCount);
			}
		};

		void Release()
		{
			for (auto& cacheEntry : bufferCache)
			{
				cacheEntry.second.usedCount = 0;
			}
		}

		lz::Buffer* GetBuffer(BufferKey bufferKey)
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

	private:
		struct BufferCacheEntry
		{
			BufferCacheEntry() : usedCount(0)
			{
			}

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
			ImageHandleInfo()
			{
			}

			ImageHandleInfo(RenderGraph* renderGraph, ImageProxyId imageProxyId)
			{
				this->renderGraph = renderGraph;
				this->imageProxyId = imageProxyId;
			}

			void Reset()
			{
				renderGraph->DeleteImage(imageProxyId);
			}

			ImageProxyId Id() const
			{
				return imageProxyId;
			}

			void SetDebugName(std::string name) const
			{
				renderGraph->SetImageProxyDebugName(imageProxyId, name);
			}

		private:
			RenderGraph* renderGraph;
			ImageProxyId imageProxyId;
			friend class RenderGraph;
		};

		struct ImageViewHandleInfo
		{
			ImageViewHandleInfo()
			{
			}

			ImageViewHandleInfo(RenderGraph* renderGraph, ImageViewProxyId imageViewProxyId)
			{
				this->renderGraph = renderGraph;
				this->imageViewProxyId = imageViewProxyId;
			}

			void Reset()
			{
				renderGraph->DeleteImageView(imageViewProxyId);
			}

			ImageViewProxyId Id() const
			{
				return imageViewProxyId;
			}

			void SetDebugName(std::string name) const
			{
				renderGraph->SetImageViewProxyDebugName(imageViewProxyId, name);
			}

		private:
			RenderGraph* renderGraph;
			ImageViewProxyId imageViewProxyId;
			friend class RenderGraph;
		};

		struct BufferHandleInfo
		{
			BufferHandleInfo()
			{
			}

			BufferHandleInfo(RenderGraph* renderGraph, BufferProxyId bufferProxyId)
			{
				this->renderGraph = renderGraph;
				this->bufferProxyId = bufferProxyId;
			}

			void Reset()
			{
				renderGraph->DeleteBuffer(bufferProxyId);
			}

			BufferProxyId Id() const
			{
				return bufferProxyId;
			}

		private:
			RenderGraph* renderGraph;
			BufferProxyId bufferProxyId;
			friend class RenderGraph;
		};

	public:
		RenderGraph(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice, vk::DispatchLoaderDynamic loader)
			: physicalDevice(physicalDevice),
			  logicalDevice(logicalDevice),
			  loader(loader),
			  renderPassCache(logicalDevice),
			  framebufferCache(logicalDevice),
			  imageCache(physicalDevice, logicalDevice, loader),
			  imageViewCache(physicalDevice, logicalDevice),
			  bufferCache(physicalDevice, logicalDevice)
		{
		}

		using ImageProxyUnique = UniqueHandle<ImageHandleInfo, RenderGraph>;
		using ImageViewProxyUnique = UniqueHandle<ImageViewHandleInfo, RenderGraph>;
		using BufferProxyUnique = UniqueHandle<BufferHandleInfo, RenderGraph>;

		ImageProxyUnique AddImage(vk::Format format, uint32_t mipsCount, uint32_t arrayLayersCount, glm::uvec2 size,
		                          vk::ImageUsageFlags usageFlags)
		{
			return AddImage(format, mipsCount, arrayLayersCount, glm::uvec3(size.x, size.y, -1), usageFlags);
		}

		ImageProxyUnique AddImage(vk::Format format, uint32_t mipsCount, uint32_t arrayLayersCount, glm::uvec3 size,
		                          vk::ImageUsageFlags usageFlags)
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

		ImageProxyUnique AddExternalImage(lz::ImageData* image)
		{
			ImageProxy imageProxy;
			imageProxy.type = ImageProxy::Types::External;
			imageProxy.externalImage = image;
			imageProxy.imageKey.debugName = "External graph image";
			return ImageProxyUnique(ImageHandleInfo(this, imageProxies.Add(std::move(imageProxy))));
		}

		ImageViewProxyUnique AddImageView(ImageProxyId imageProxyId, uint32_t baseMipLevel, uint32_t mipLevelsCount,
		                                  uint32_t baseArrayLayer, uint32_t arrayLayersCount)
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

		ImageViewProxyUnique AddExternalImageView(lz::ImageView* imageView,
		                                          lz::ImageUsageTypes usageType = lz::ImageUsageTypes::Unknown)
		{
			ImageViewProxy imageViewProxy;
			imageViewProxy.externalView = imageView;
			imageViewProxy.externalUsageType = usageType;
			imageViewProxy.type = ImageViewProxy::Types::External;
			imageViewProxy.imageProxyId = ImageProxyId();
			imageViewProxy.debugName = "External view";
			return ImageViewProxyUnique(ImageViewHandleInfo(this, imageViewProxies.Add(std::move(imageViewProxy))));
		}

	private:
		void DeleteImage(ImageProxyId imageId)
		{
			imageProxies.Release(imageId);
		}

		void SetImageProxyDebugName(ImageProxyId imageId, std::string debugName)
		{
			imageProxies.Get(imageId).imageKey.debugName = debugName;
		}

		void SetImageViewProxyDebugName(ImageViewProxyId imageViewId, std::string debugName)
		{
			imageViewProxies.Get(imageViewId).debugName = debugName;
		}

		void DeleteImageView(ImageViewProxyId imageViewId)
		{
			imageViewProxies.Release(imageViewId);
		}

		void DeleteBuffer(BufferProxyId bufferId)
		{
			bufferProxies.Release(bufferId);
		}

	public:
		glm::uvec2 GetMipSize(ImageProxyId imageProxyId, uint32_t mipLevel)
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

		glm::uvec2 GetMipSize(ImageViewProxyId imageViewProxyId, uint32_t mipOffset)
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

		template <typename BufferType>
		BufferProxyUnique AddBuffer(uint32_t count)
		{
			BufferProxy bufferProxy;
			bufferProxy.type = BufferProxy::Types::Transient;
			bufferProxy.bufferKey.elementSize = sizeof(BufferType);
			bufferProxy.bufferKey.elementsCount = count;
			bufferProxy.externalBuffer = nullptr;
			return BufferProxyUnique(BufferHandleInfo(this, bufferProxies.Add(std::move(bufferProxy))));
		}

		BufferProxyUnique AddExternalBuffer(lz::Buffer* buffer)
		{
			BufferProxy bufferProxy;
			bufferProxy.type = BufferProxy::Types::External;
			bufferProxy.bufferKey.elementSize = -1;
			bufferProxy.bufferKey.elementsCount = -1;
			bufferProxy.externalBuffer = buffer;
			return BufferProxyUnique(BufferHandleInfo(this, bufferProxies.Add(std::move(bufferProxy))));
		}

		struct PassContext
		{
			lz::ImageView* GetImageView(ImageViewProxyId imageViewProxyId)
			{
				return resolvedImageViews[imageViewProxyId.asInt];
			}

			lz::Buffer* GetBuffer(BufferProxyId bufferProxy)
			{
				return resolvedBuffers[bufferProxy.asInt];
			}

			vk::CommandBuffer GetCommandBuffer()
			{
				return commandBuffer;
			}

		private:
			std::vector<lz::ImageView*> resolvedImageViews;
			std::vector<lz::Buffer*> resolvedBuffers;
			vk::CommandBuffer commandBuffer;
			friend class RenderGraph;
		};

		struct RenderPassContext : public PassContext
		{
			lz::RenderPass* GetRenderPass()
			{
				return renderPass;
			}

		private:
			lz::RenderPass* renderPass;
			friend class RenderGraph;
		};

		struct RenderPassDesc
		{
			RenderPassDesc()
			{
				profilerTaskName = "RenderPass";
				profilerTaskColor = glm::packUnorm4x8(glm::vec4(1.0f, 0.5f, 0.0f, 1.0f));
			}

			struct Attachment
			{
				ImageViewProxyId imageViewProxyId;
				vk::AttachmentLoadOp loadOp;
				vk::ClearValue clearValue;
			};

			RenderPassDesc& SetColorAttachments(
				const std::vector<ImageViewProxyId>& colorAttachmentViewProxies,
				vk::AttachmentLoadOp loadop = vk::AttachmentLoadOp::eDontCare,
				vk::ClearValue clearValue = vk::ClearColorValue(std::array<float, 4>{1.0f, 0.5f, 0.0f, 1.0f}))
			{
				this->colorAttachments.resize(colorAttachmentViewProxies.size());
				for (size_t index =0 ; index < colorAttachmentViewProxies.size(); ++index)
				{
					this->colorAttachments[index] = { colorAttachmentViewProxies[index], loadop, clearValue };
				}
				return *this;
			}

			RenderPassDesc& SetColorAttachments(std::vector<Attachment>&& _colorAttachments)
			{
				this->colorAttachments = std::move(_colorAttachments);
				return *this;
			}

			RenderPassDesc& SetDepthAttachment(
				ImageViewProxyId _depthAttachmentViewProxyId,
				vk::AttachmentLoadOp _loadOp = vk::AttachmentLoadOp::eDontCare,
				vk::ClearValue _clearValue = vk::ClearDepthStencilValue(1.0f, 0))
			{
				this->depthAttachment.imageViewProxyId = _depthAttachmentViewProxyId;
				this->depthAttachment.loadOp = _loadOp;
				this->depthAttachment.clearValue = _clearValue;
				return *this;
			}
			RenderPassDesc& SetDepthAttachment(Attachment _depthAttachment)
			{
				this->depthAttachment = _depthAttachment;
				return *this;
			}

			RenderPassDesc& SetVertexBuffers(std::vector<BufferProxyId>&& _vertexBufferProxies)
			{
				this->vertexBufferProxies = std::move(_vertexBufferProxies);
				return *this;
			}

			RenderPassDesc& SetInputImages(std::vector<ImageViewProxyId>&& _inputImageViewProxies)
			{
				this->inputImageViewProxies = std::move(_inputImageViewProxies);
				return *this;
			}
			RenderPassDesc& SetStorageBuffers(std::vector<BufferProxyId>&& _inoutStorageBufferProxies)
			{
				this->inoutStorageBufferProxies = _inoutStorageBufferProxies;
				return *this;
			}
			RenderPassDesc& SetStorageImages(std::vector<ImageViewProxyId>&& _inoutStorageImageProxies)
			{
				this->inoutStorageImageProxies = _inoutStorageImageProxies;
				return *this;
			}
			RenderPassDesc& SetRenderAreaExtent(vk::Extent2D _renderAreaExtent)
			{
				this->renderAreaExtent = _renderAreaExtent;
				return *this;
			}

			RenderPassDesc& SetRecordFunc(std::function<void(RenderPassContext)> _recordFunc)
			{
				this->recordFunc = _recordFunc;
				return *this;
			}
			RenderPassDesc& SetProfilerInfo(uint32_t taskColor, std::string taskName)
			{
				this->profilerTaskColor = taskColor;
				this->profilerTaskName = taskName;
				return *this;
			}

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
			std::function<void(RenderPassContext)> recordFunc)
		{
			RenderPassDesc renderPassDesc;
			for (const auto& proxy : colorAttachmentImageProxies)
			{
				RenderPassDesc::Attachment colorAttachment = { proxy, loadOp, vk::ClearColorValue(std::array<float, 4>{0.03f, 0.03f, 0.03f, 1.0f}) };
				renderPassDesc.colorAttachments.push_back(colorAttachment);
			}
			renderPassDesc.depthAttachment = { depthAttachmentImageProxy, loadOp, vk::ClearDepthStencilValue(1.0f, 0) };
			renderPassDesc.inputImageViewProxies = inputImageViewProxies;
			renderPassDesc.inoutStorageBufferProxies = {};
			renderPassDesc.renderAreaExtent = renderAreaExtent;
			renderPassDesc.recordFunc = recordFunc;

			AddPass(renderPassDesc);
		}

		void AddPass(RenderPassDesc& renderPassDesc)
		{
			Task task;
			task.type = Task::Types::RenderPass;
			task.index = renderPassDescs.size();
			AddTask(task);

			renderPassDescs.emplace_back(renderPassDesc);
		}

		void Clear()
		{
			*this = RenderGraph(physicalDevice, logicalDevice, loader);
		}

		struct ComputePassDesc
		{
			ComputePassDesc()
			{
				profilerTaskName = "ComputePass";
				profilerTaskColor = lz::Colors::belizeHole;
			}
			ComputePassDesc& SetInputImages(std::vector<ImageViewProxyId>&& _inputImageViewProxies)
			{
				this->inputImageViewProxies = std::move(_inputImageViewProxies);
				return *this;
			}
			ComputePassDesc& SetStorageBuffers(std::vector<BufferProxyId>&& _inoutStorageBufferProxies)
			{
				this->inoutStorageBufferProxies = _inoutStorageBufferProxies;
				return *this;
			}
			ComputePassDesc& SetStorageImages(std::vector<ImageViewProxyId>&& _inoutStorageImageProxies)
			{
				this->inoutStorageImageProxies = _inoutStorageImageProxies;
				return *this;
			}
			ComputePassDesc& SetRecordFunc(std::function<void(PassContext)> _recordFunc)
			{
				this->recordFunc = _recordFunc;
				return *this;
			}
			ComputePassDesc& SetProfilerInfo(uint32_t taskColor, std::string taskName)
			{
				this->profilerTaskColor = taskColor;
				this->profilerTaskName = taskName;
				return *this;
			}
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
			std::function<void(PassContext)> recordFunc)
		{
			ComputePassDesc computePassDesc;
			computePassDesc.inoutStorageBufferProxies = inoutBufferProxies;
			computePassDesc.inputImageViewProxies = inputImageViewProxies;
			computePassDesc.recordFunc = recordFunc;

			AddPass(computePassDesc);
		}

		void AddPass(ComputePassDesc& computePassDesc)
		{
			Task task;
			task.type = Task::Types::ComputePass;
			task.index = computePassDescs.size();
			AddTask(task);

			computePassDescs.emplace_back(computePassDesc);
		}


		struct TransferPassDesc
		{
			TransferPassDesc()
			{
				profilerTaskName = "TransferPass";
				profilerTaskColor = lz::Colors::silver;
			}
			TransferPassDesc& SetSrcImages(std::vector<ImageViewProxyId>&& _srcImageViewProxies)
			{
				this->srcImageViewProxies = std::move(_srcImageViewProxies);
				return *this;
			}

			TransferPassDesc& SetDstImages(std::vector<ImageViewProxyId>&& _dstImageViewProxies)
			{
				this->dstImageViewProxies = std::move(_dstImageViewProxies);
				return *this;
			}

			TransferPassDesc& SetSrcBuffers(std::vector<BufferProxyId>&& _srcBufferProxies)
			{
				this->srcBufferProxies = std::move(_srcBufferProxies);
				return *this;
			}

			TransferPassDesc& SetDstBuffers(std::vector<BufferProxyId>&& _dstBufferProxies)
			{
				this->dstBufferProxies = std::move(_dstBufferProxies);
				return *this;
			}

			TransferPassDesc& SetRecordFunc(std::function<void(PassContext)> _recordFunc)
			{
				this->recordFunc = _recordFunc;
				return *this;
			}

			TransferPassDesc& SetProfilerInfo(uint32_t taskColor, std::string taskName)
			{
				this->profilerTaskColor = taskColor;
				this->profilerTaskName = taskName;
				return *this;
			}

			std::vector<BufferProxyId> srcBufferProxies;
			std::vector<ImageViewProxyId> srcImageViewProxies;

			std::vector<BufferProxyId> dstBufferProxies;
			std::vector<ImageViewProxyId> dstImageViewProxies;

			std::function<void(PassContext)> recordFunc;

			std::string profilerTaskName;
			uint32_t profilerTaskColor;
		};

		void AddPass(TransferPassDesc& transferPassDesc)
		{
			Task task;
			task.type = Task::Types::TransferPass;
			task.index = transferPassDescs.size();
			AddTask(task);

			transferPassDescs.emplace_back(transferPassDesc);
		}

		struct ImagePresentPassDesc
		{
			ImagePresentPassDesc& SetImage(ImageViewProxyId _presentImageViewProxyId)
			{
				this->presentImageViewProxyId = _presentImageViewProxyId;
			}
			ImageViewProxyId presentImageViewProxyId;
		};
		void AddPass(ImagePresentPassDesc&& imagePresentDesc)
		{
			Task task;
			task.type = Task::Types::ImagePresent;
			task.index = imagePresentDescs.size();
			AddTask(task);

			imagePresentDescs.push_back(imagePresentDesc);
		}
		void AddImagePresent(ImageViewProxyId presentImageViewProxyId)
		{
			ImagePresentPassDesc imagePresentDesc;
			imagePresentDesc.presentImageViewProxyId = presentImageViewProxyId;

			AddPass(std::move(imagePresentDesc));
		}

		struct FrameSyncBeginPassDesc
		{
		};
		struct FrameSyncEndPassDesc
		{
		};

		void AddPass(FrameSyncBeginPassDesc&& frameSyncBeginDesc)
		{
			Task task;
			task.type = Task::Types::FrameSyncBegin;
			task.index = frameSyncBeginDescs.size();
			AddTask(task);

			frameSyncBeginDescs.push_back(frameSyncBeginDesc);
		}

		void AddPass(FrameSyncEndPassDesc&& frameSyncEndDesc)
		{
			Task task;
			task.type = Task::Types::FrameSyncEnd;
			task.index = frameSyncEndDescs.size();
			AddTask(task);

			frameSyncEndDescs.push_back(frameSyncEndDesc);
		}

		void Execute(vk::CommandBuffer commandBuffer, lz::CpuProfiler* cpuProfiler, lz::GpuProfiler* gpuProfiler)
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
					}
				}

			}
		}

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

			bool Contains(const ImageViewProxy& other)
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

		void ResolveImages()
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

		lz::ImageData* GetResolvedImage(size_t taskIndex, ImageProxyId imageProxy)
		{
			return imageProxies.Get(imageProxy).resolvedImage;
		}

		ImageViewCache imageViewCache;
		ImageViewProxyPool imageViewProxies;

		void ResolveImageViews()
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

		lz::ImageView* GetResolvedImageView(size_t taskIndex, ImageViewProxyId imageViewProxyId)
		{
			return imageViewProxies.Get(imageViewProxyId).resolvedImageView;
		}


		BufferCache bufferCache;
		BufferProxyPool bufferProxies;

		void ResolveBuffers()
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

		lz::Buffer* GetResolvedBuffer(size_t taskIndex, BufferProxyId bufferProxyId)
		{
			return bufferProxies.Get(bufferProxyId).resolvedBuffer;
		}

		std::vector<Task> tasks;

		void AddTask(Task task)
		{
			tasks.push_back(task);
		}

		lz::ProfilerTask CreateProfilerTask(const RenderPassDesc& renderPassDesc)
		{
			lz::ProfilerTask task;
			task.startTime = -1.0f;
			task.endTime = -1.0f;
			task.name = renderPassDesc.profilerTaskName;
			task.color = renderPassDesc.profilerTaskColor;
			return task;
		}

		lz::ProfilerTask CreateProfilerTask(const ComputePassDesc& computePassDesc)
		{
			lz::ProfilerTask task;
			task.startTime = -1.0f;
			task.endTime = -1.0f;
			task.name = computePassDesc.profilerTaskName;
			task.color = computePassDesc.profilerTaskColor;
			return task;
		}

		lz::ProfilerTask CreateProfilerTask(const TransferPassDesc& transferPassDesc)
		{
			lz::ProfilerTask task;
			task.startTime = -1.0f;
			task.endTime = -1.0f;
			task.name = transferPassDesc.profilerTaskName;
			task.color = transferPassDesc.profilerTaskColor;
			return task;
		}

		lz::ProfilerTask CreateProfilerTask(const ImagePresentPassDesc& imagePresentPassDesc)
		{
			lz::ProfilerTask task;
			task.startTime = -1.0f;
			task.endTime = -1.0f;
			task.name = "ImagePresent";
			task.color = glm::packUnorm4x8(glm::vec4(0.0f, 1.0f, 0.5f, 1.0f));
			return task;
		}

		lz::ProfilerTask CreateProfilerTask(const FrameSyncBeginPassDesc& frameSyncBeginPassDesc)
		{
			lz::ProfilerTask task;
			task.startTime = -1.0f;
			task.endTime = -1.0f;
			task.name = "FrameSyncBegin";
			task.color = glm::packUnorm4x8(glm::vec4(0.0f, 0.5f, 1.0f, 1.0f));
			return task;
		}

		lz::ProfilerTask CreateProfilerTask(const FrameSyncEndPassDesc& frameSyncEndPassDesc)
		{
			lz::ProfilerTask task;
			task.startTime = -1.0f;
			task.endTime = -1.0f;
			task.name = "FrameSyncEnd";
			task.color = glm::packUnorm4x8(glm::vec4(0.0f, 0.5f, 1.0f, 1.0f));
			return task;
		}

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
