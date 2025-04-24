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
			id_(size_t(-1))
		{
		}

		bool operator ==(const RenderGraphProxyId<Base>& other) const
		{
			return this->id_ == other.id_;
		}

		bool is_valid() const
		{
			return id_ != size_t(-1);
		}

	private:
		explicit RenderGraphProxyId(size_t _id) : id_(_id)
		{
		}

		size_t id_;
		friend class RenderGraph;
	};

	class ImageCache
	{
	public:
		struct ImageKey
		{
			bool operator <(const ImageKey& other) const;

			vk::Format format;
			vk::ImageUsageFlags usage_flags;
			uint32_t mips_count;
			uint32_t array_layers_count;
			glm::uvec3 size;
			std::string debug_name;
		};

		ImageCache(vk::PhysicalDevice physical_device, vk::Device logical_device, vk::DispatchLoaderDynamic loader);

		void release();

		lz::ImageData* get_image(ImageKey image_key);

	private:
		struct ImageCacheEntry
		{
			ImageCacheEntry();

			std::vector<std::unique_ptr<lz::Image>> images;
			size_t used_count;
		};

		std::map<ImageKey, ImageCacheEntry> image_cache_;
		vk::PhysicalDevice physical_device_;
		vk::Device logical_device_;
		vk::DispatchLoaderDynamic loader_;
	};

	class ImageViewCache
	{
	public:
		struct ImageViewKey
		{
			lz::ImageData* image;
			ImageSubresourceRange subresource_range;
			std::string debug_name;

			bool operator <(const ImageViewKey& other) const;
		};

		ImageViewCache(vk::PhysicalDevice physical_device, vk::Device logical_device);

		lz::ImageView* get_image_view(ImageViewKey image_view_key);

	private:
		std::map<ImageViewKey, std::unique_ptr<lz::ImageView>> image_view_cache_;
		vk::PhysicalDevice physical_device_;
		vk::Device logical_device_;
	};

	class BufferCache
	{
	public:
		BufferCache(vk::PhysicalDevice physical_device, vk::Device logical_device);

		struct BufferKey
		{
			uint32_t element_size;
			uint32_t elements_count;

			bool operator <(const BufferKey& other) const;
		};

		void release();

		lz::Buffer* get_buffer(BufferKey buffer_key);

	private:
		struct BufferCacheEntry
		{
			BufferCacheEntry();

			std::vector<std::unique_ptr<lz::Buffer>> buffers;
			size_t used_count;
		};

		std::map<BufferKey, BufferCacheEntry> buffer_cache_;
		vk::PhysicalDevice physical_device_;
		vk::Device logical_device_;
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

			ImageHandleInfo(RenderGraph* render_graph, ImageProxyId image_proxy_id);

			void reset();

			ImageProxyId id() const;

			void set_debug_name(std::string name) const;

		private:
			RenderGraph* render_graph_;
			ImageProxyId image_proxy_id_;
			friend class RenderGraph;
		};

		struct ImageViewHandleInfo
		{
			ImageViewHandleInfo();

			ImageViewHandleInfo(RenderGraph* render_graph, ImageViewProxyId image_view_proxy_id);

			void reset();

			ImageViewProxyId id() const;

			void set_debug_name(std::string name) const;

		private:
			RenderGraph* render_graph_;
			ImageViewProxyId image_view_proxy_id_;
			friend class RenderGraph;
		};

		struct BufferHandleInfo
		{
			BufferHandleInfo();

			BufferHandleInfo(RenderGraph* render_graph, BufferProxyId buffer_proxy_id);

			void reset();

			BufferProxyId id() const;

		private:
			RenderGraph* render_graph_;
			BufferProxyId buffer_proxy_id_;
			friend class RenderGraph;
		};

	public:
		RenderGraph(vk::PhysicalDevice physical_device, vk::Device logical_device, vk::DispatchLoaderDynamic loader);

		using ImageProxyUnique = UniqueHandle<ImageHandleInfo, RenderGraph>;
		using ImageViewProxyUnique = UniqueHandle<ImageViewHandleInfo, RenderGraph>;
		using BufferProxyUnique = UniqueHandle<BufferHandleInfo, RenderGraph>;

		ImageProxyUnique add_image(vk::Format format, uint32_t mips_count, uint32_t array_layers_count, glm::uvec2 size,
		                           vk::ImageUsageFlags usage_flags);

		ImageProxyUnique add_image(vk::Format format, uint32_t mips_count, uint32_t array_layers_count, glm::uvec3 size,
		                           vk::ImageUsageFlags usage_flags);

		ImageProxyUnique add_external_image(lz::ImageData* image);

		ImageViewProxyUnique add_image_view(ImageProxyId image_proxy_id, uint32_t base_mip_level,
		                                    uint32_t mip_levels_count,
		                                    uint32_t base_array_layer, uint32_t array_layers_count);

		ImageViewProxyUnique add_external_image_view(lz::ImageView* image_view,
		                                             lz::ImageUsageTypes usage_type = lz::ImageUsageTypes::eUnknown);

	private:
		void delete_image(ImageProxyId image_id);

		void set_image_proxy_debug_name(ImageProxyId image_id, std::string debug_name);

		void set_image_view_proxy_debug_name(ImageViewProxyId image_view_id, std::string debug_name);

		void delete_image_view(ImageViewProxyId image_view_id);

		void delete_buffer(BufferProxyId buffer_id);

	public:
		glm::uvec2 get_mip_size(ImageProxyId image_proxy_id, uint32_t mip_level);

		glm::uvec2 get_mip_size(ImageViewProxyId image_view_proxy_id, uint32_t mip_offset);

		template <typename BufferType>
		RenderGraph::BufferProxyUnique add_buffer(const uint32_t count)
		{
			BufferProxy buffer_proxy;
			buffer_proxy.type = BufferProxy::Types::eTransient;
			buffer_proxy.buffer_key.element_size = sizeof(BufferType);
			buffer_proxy.buffer_key.elements_count = count;
			buffer_proxy.external_buffer = nullptr;
			return BufferProxyUnique(BufferHandleInfo(this, buffer_proxies_.add(std::move(buffer_proxy))));
		}

		BufferProxyUnique add_external_buffer(lz::Buffer* buffer);

		struct PassContext
		{
			lz::ImageView* get_image_view(ImageViewProxyId image_view_proxy_id) const;

			lz::Buffer* get_buffer(BufferProxyId buffer_proxy) const;

			vk::CommandBuffer get_command_buffer() const;

		private:
			std::vector<lz::ImageView*> resolved_image_views_;
			std::vector<lz::Buffer*> resolved_buffers_;
			vk::CommandBuffer command_buffer_;
			friend class RenderGraph;
		};

		struct RenderPassContext : public PassContext
		{
			lz::RenderPass* get_render_pass() const;

		private:
			lz::RenderPass* render_pass_;
			friend class RenderGraph;
		};

		struct RenderPassDesc
		{
			RenderPassDesc();

			struct Attachment
			{
				ImageViewProxyId image_view_proxy_id;
				vk::AttachmentLoadOp load_op;
				vk::ClearValue clear_value;
			};

			RenderPassDesc& set_color_attachments(
				const std::vector<ImageViewProxyId>& color_attachment_view_proxies,
				vk::AttachmentLoadOp load_op = vk::AttachmentLoadOp::eDontCare,
				vk::ClearValue clear_value = vk::ClearColorValue(std::array<float, 4>{1.0f, 0.5f, 0.0f, 1.0f}));

			RenderPassDesc& set_color_attachments(std::vector<Attachment>&& color_attachments);

			RenderPassDesc& set_depth_attachment(
				ImageViewProxyId depth_attachment_view_proxy_id,
				vk::AttachmentLoadOp load_op = vk::AttachmentLoadOp::eDontCare,
				vk::ClearValue _clearValue = vk::ClearDepthStencilValue(1.0f, 0));

			RenderPassDesc& set_depth_attachment(Attachment depth_attachment);

			RenderPassDesc& set_vertex_buffers(std::vector<BufferProxyId>&& vertex_buffer_proxies);

			RenderPassDesc& set_input_images(std::vector<ImageViewProxyId>&& input_image_view_proxies);

			RenderPassDesc& set_storage_buffers(std::vector<BufferProxyId>&& inout_storage_buffer_proxies);

			RenderPassDesc& set_storage_images(std::vector<ImageViewProxyId>&& inout_storage_image_proxies);

			RenderPassDesc& set_render_area_extent(vk::Extent2D render_area_extent);

			RenderPassDesc& set_record_func(std::function<void(RenderPassContext)> record_func);

			RenderPassDesc& set_profiler_info(uint32_t task_color, std::string task_name);

			std::vector<Attachment> color_attachments;
			Attachment depth_attachment;

			std::vector<ImageViewProxyId> input_image_view_proxies;
			std::vector<BufferProxyId> vertex_buffer_proxies;
			std::vector<BufferProxyId> inout_storage_buffer_proxies;
			std::vector<ImageViewProxyId> inout_storage_image_proxies;

			vk::Extent2D render_area_extent;
			std::function<void(RenderPassContext)> record_func;

			std::string profiler_task_name;
			uint32_t profiler_task_color;
		};

		void add_render_pass(
			std::vector<ImageViewProxyId> color_attachment_image_proxies,
			ImageViewProxyId depth_attachment_image_proxy,
			std::vector<ImageViewProxyId> input_image_view_proxies,
			vk::Extent2D render_area_extent,
			vk::AttachmentLoadOp load_op,
			std::function<void(RenderPassContext)> record_func);

		void add_pass(RenderPassDesc& render_pass_desc);

		void clear();

		struct ComputePassDesc
		{
			ComputePassDesc();

			ComputePassDesc& set_input_images(std::vector<ImageViewProxyId>&& input_image_view_proxies);
			ComputePassDesc& set_storage_buffers(std::vector<BufferProxyId>&& inout_storage_buffer_proxies);
			ComputePassDesc& set_storage_images(std::vector<ImageViewProxyId>&& inout_storage_image_proxies);
			ComputePassDesc& set_record_func(std::function<void(PassContext)> record_func);
			ComputePassDesc& set_profiler_info(uint32_t task_color, std::string task_name);

			std::vector<BufferProxyId> inout_storage_buffer_proxies;
			std::vector<ImageViewProxyId> input_image_view_proxies;
			std::vector<ImageViewProxyId> inout_storage_image_proxies;

			std::function<void(PassContext)> record_func;

			std::string profiler_task_name;
			uint32_t profiler_task_color;
		};

		void add_compute_pass(
			std::vector<BufferProxyId> inout_buffer_proxies,
			std::vector<ImageViewProxyId> input_image_view_proxies,
			std::function<void(PassContext)> record_func);

		void add_pass(ComputePassDesc& compute_pass_desc);


		struct TransferPassDesc
		{
			TransferPassDesc();

			TransferPassDesc& set_src_images(std::vector<ImageViewProxyId>&& src_image_view_proxies);
			TransferPassDesc& set_dst_images(std::vector<ImageViewProxyId>&& dst_image_view_proxies);
			TransferPassDesc& set_src_buffers(std::vector<BufferProxyId>&& src_buffer_proxies);
			TransferPassDesc& set_dst_buffers(std::vector<BufferProxyId>&& dst_buffer_proxies);
			TransferPassDesc& set_record_func(std::function<void(PassContext)> record_func);
			TransferPassDesc& set_profiler_info(uint32_t task_color, std::string task_name);

			std::vector<BufferProxyId> src_buffer_proxies;
			std::vector<ImageViewProxyId> src_image_view_proxies;

			std::vector<BufferProxyId> dst_buffer_proxies;
			std::vector<ImageViewProxyId> dst_image_view_proxies;

			std::function<void(PassContext)> record_func;

			std::string profiler_task_name;
			uint32_t profiler_task_color;
		};

		void add_pass(TransferPassDesc& transfer_pass_desc);

		struct ImagePresentPassDesc
		{
			ImagePresentPassDesc& set_image(ImageViewProxyId present_image_view_proxy_id);

			ImageViewProxyId present_image_view_proxy_id;
		};

		void add_pass(ImagePresentPassDesc&& image_present_desc);

		void add_image_present(ImageViewProxyId present_image_view_proxy_id);

		struct FrameSyncBeginPassDesc
		{
		};

		struct FrameSyncEndPassDesc
		{
		};

		void add_pass(FrameSyncBeginPassDesc&& frame_sync_begin_desc);

		void add_pass(FrameSyncEndPassDesc&& frame_sync_end_desc);

		void execute(vk::CommandBuffer command_buffer, lz::CpuProfiler* cpu_profiler, lz::GpuProfiler* gpu_profiler);

	private:
		void flush_external_images(vk::CommandBuffer command_buffer, lz::CpuProfiler* cpu_profiler,
		                         lz::GpuProfiler* gpu_profiler);

		static bool image_view_contains_subresource(lz::ImageView* image_view, lz::ImageData* image_data, uint32_t mip_level,
		                                            uint32_t array_layer);

		ImageUsageTypes get_task_image_subresource_usage_type(size_t task_index, lz::ImageData* image_data, uint32_t mip_level,
		                                                 uint32_t array_layer);

		BufferUsageTypes get_task_buffer_usage_type(size_t task_index, lz::Buffer* buffer);

		ImageUsageTypes get_last_image_subresource_usage_type(size_t task_index, lz::ImageData* image_data, uint32_t mip_level,
		                                                 uint32_t array_layer);

		BufferUsageTypes get_last_buffer_usage_type(size_t task_index, lz::Buffer* buffer);

		void flush_image_transition_barriers(lz::ImageData* image_data, vk::ImageSubresourceRange range,
		                                  ImageUsageTypes src_usage_type, ImageUsageTypes dst_usage_type,
		                                  vk::PipelineStageFlags& src_stage, vk::PipelineStageFlags& dst_stage,
		                                  std::vector<vk::ImageMemoryBarrier>& image_barriers);

		void add_image_transition_barriers(lz::ImageView* image_view, ImageUsageTypes dst_usage_type, size_t dst_task_index,
		                                vk::PipelineStageFlags& src_stage, vk::PipelineStageFlags& dst_stage,
		                                std::vector<vk::ImageMemoryBarrier>& image_barriers);

		void flush_buffer_transition_barriers(lz::Buffer* buffer, BufferUsageTypes src_usage_type,
		                                   BufferUsageTypes dst_usage_type, vk::PipelineStageFlags& src_stage,
		                                   vk::PipelineStageFlags& dst_stage,
		                                   std::vector<vk::BufferMemoryBarrier>& buffer_barriers);

		void add_buffer_barriers(lz::Buffer* buffer, BufferUsageTypes dstUsageType, size_t dst_task_index,
		                       vk::PipelineStageFlags& src_stage, vk::PipelineStageFlags& dst_stage,
		                       std::vector<vk::BufferMemoryBarrier>& buffer_barriers);

	private:
		struct ImageProxy
		{
			enum struct Types : uint8_t
			{
				eTransient,
				eExternal
			};

			ImageCache::ImageKey image_key;
			lz::ImageData* external_image;

			lz::ImageData* resolved_image;

			Types type;
		};

		struct ImageViewProxy
		{
			enum struct Types : uint8_t
			{
				eTransient,
				eExternal
			};

			bool contains(const ImageViewProxy& other);

			ImageProxyId image_proxy_id;
			ImageSubresourceRange subresource_range;

			lz::ImageView* external_view;
			lz::ImageUsageTypes external_usage_type;

			lz::ImageView* resolved_image_view;
			std::string debug_name;

			Types type;
		};

		struct BufferProxy
		{
			enum struct Types : uint8_t
			{
				eTransient,
				eExternal
			};

			BufferCache::BufferKey buffer_key;
			lz::Buffer* external_buffer;
			lz::Buffer* resolved_buffer;

			Types type;
		};

		struct Task
		{
			enum struct Types : uint8_t
			{
				eRenderPass,
				eComputePass,
				eTransferPass,
				eImagePresent,
				eFrameSyncBegin,
				eFrameSyncEnd
			};

			Types type;
			size_t index;
		};

		ImageCache image_cache_;
		ImageProxyPool image_proxies_;

		void resolve_images();

		lz::ImageData* get_resolved_image(size_t task_index, ImageProxyId image_proxy);

		ImageViewCache image_view_cache_;
		ImageViewProxyPool image_view_proxies_;
		void resolve_image_views();
		lz::ImageView* get_resolved_image_view(size_t task_index, ImageViewProxyId image_view_proxy_id);


		BufferCache buffer_cache_;
		BufferProxyPool buffer_proxies_;
		void resolve_buffers();
		lz::Buffer* get_resolved_buffer(size_t task_index, BufferProxyId buffer_proxy_id);

		std::vector<Task> tasks_;
		void add_task(Task task);
		lz::ProfilerTask create_profiler_task(const RenderPassDesc& render_pass_desc);
		lz::ProfilerTask create_profiler_task(const ComputePassDesc& compute_pass_desc);
		lz::ProfilerTask create_profiler_task(const TransferPassDesc& transfer_pass_desc);
		lz::ProfilerTask create_profiler_task(const ImagePresentPassDesc& image_present_pass_desc);
		lz::ProfilerTask create_profiler_task(const FrameSyncBeginPassDesc& frame_sync_begin_pass_desc);
		lz::ProfilerTask create_profiler_task(const FrameSyncEndPassDesc& frame_sync_end_pass_desc);

		RenderPassCache render_pass_cache_;
		FramebufferCache framebuffer_cache_;

		std::vector<RenderPassDesc> render_pass_descs_;
		std::vector<ComputePassDesc> compute_pass_descs_;
		std::vector<TransferPassDesc> transfer_pass_descs_;
		std::vector<ImagePresentPassDesc> image_present_descs_;
		std::vector<FrameSyncBeginPassDesc> frame_sync_begin_descs_;
		std::vector<FrameSyncEndPassDesc> frame_sync_end_descs_;


		vk::Device logical_device_;
		vk::PhysicalDevice physical_device_;
		vk::DispatchLoaderDynamic loader_;
		size_t image_allocations_ = 0;
	};
}
