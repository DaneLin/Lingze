#include "RenderGraph.h"

#include "Buffer.h"
#include "Core.h"
#include "GpuProfiler.h"
#include "ImageView.h"

namespace lz
{
bool ImageCache::ImageKey::operator<(const ImageKey &other) const
{
	auto l_usage_flags = VkMemoryMapFlags(usage_flags);
	auto r_usage_flags = VkMemoryMapFlags(other.usage_flags);
	return std::tie(format, mips_count, array_layers_count, l_usage_flags, size.x, size.y, size.z) <
	       std::tie(other.format, other.mips_count, other.array_layers_count, r_usage_flags, other.size.x, other.size.y, other.size.z);
}

ImageCache::ImageCache(vk::PhysicalDevice physical_device, vk::Device logical_device,
                       vk::DispatchLoaderDynamic loader) :
    physical_device_(physical_device),
    logical_device_(logical_device),
    loader_(loader)
{
}

void ImageCache::release()
{
	for (auto &cache_entry : image_cache_)
	{
		cache_entry.second.used_count = 0;
	}
}

lz::ImageData *ImageCache::get_image(ImageKey image_key)
{
	auto &cache_entry = image_cache_[image_key];
	if (cache_entry.used_count + 1 > cache_entry.images.size())
	{
		vk::ImageCreateInfo image_create_info;
		if (image_key.size.z == glm::u32(-1))
		{
			image_create_info = lz::Image::create_info_2d(glm::uvec2(image_key.size.x, image_key.size.y),
			                                              image_key.mips_count,
			                                              image_key.array_layers_count,
			                                              image_key.format,
			                                              image_key.usage_flags);
		}
		else
		{
			image_create_info = lz::Image::create_info_volume(image_key.size, image_key.mips_count,
			                                                  image_key.array_layers_count, image_key.format,
			                                                  image_key.usage_flags);
		}

		auto new_image = std::make_unique<lz::Image>(physical_device_, logical_device_, image_create_info);
		Core::set_object_debug_name(logical_device_, loader_, new_image->get_image_data()->get_handle(),
		                            image_key.debug_name);
		cache_entry.images.emplace_back(std::move(new_image));
	}
	return cache_entry.images[cache_entry.used_count++]->get_image_data();
}

ImageCache::ImageCacheEntry::ImageCacheEntry() :
    used_count(0)
{
}

bool ImageViewCache::ImageViewKey::operator<(const ImageViewKey &other) const
{
	return std::tie(image, subresource_range) < std::tie(other.image, other.subresource_range);
}

ImageViewCache::ImageViewCache(vk::PhysicalDevice physical_device, vk::Device logical_device) :
    physical_device_(physical_device), logical_device_(logical_device)
{
}

lz::ImageView *ImageViewCache::get_image_view(ImageViewKey image_view_key)
{
	auto &image_view = image_view_cache_[image_view_key];
	if (!image_view)
		image_view = std::make_unique<lz::ImageView>(
		    logical_device_,
		    image_view_key.image,
		    image_view_key.subresource_range.base_mip_level,
		    image_view_key.subresource_range.mips_count,
		    image_view_key.subresource_range.base_array_layer,
		    image_view_key.subresource_range.array_layers_count);
	return image_view.get();
}

BufferCache::BufferCache(vk::PhysicalDevice physical_device, vk::Device logical_device) :
    physical_device_(physical_device),
    logical_device_(logical_device)
{
}

bool BufferCache::BufferKey::operator<(const BufferKey &other) const
{
	return std::tie(element_size, elements_count) < std::tie(other.element_size, other.elements_count);
}

void BufferCache::release()
{
	for (auto &cache_entry : buffer_cache_)
	{
		cache_entry.second.used_count = 0;
	}
}

lz::Buffer *BufferCache::get_buffer(BufferKey buffer_key)
{
	auto &cache_entry = buffer_cache_[buffer_key];
	if (cache_entry.used_count + 1 > cache_entry.buffers.size())
	{
		auto new_buffer = std::make_unique<lz::Buffer>(
		    physical_device_,
		    logical_device_,
		    buffer_key.element_size * buffer_key.elements_count,
		    vk::BufferUsageFlagBits::eStorageBuffer,
		    vk::MemoryPropertyFlagBits::eDeviceLocal);
		cache_entry.buffers.emplace_back(std::move(new_buffer));
	}
	return cache_entry.buffers[cache_entry.used_count++].get();
}

BufferCache::BufferCacheEntry::BufferCacheEntry() :
    used_count(0)
{
}

RenderGraph::ImageHandleInfo::ImageHandleInfo()
{
}

RenderGraph::ImageHandleInfo::ImageHandleInfo(RenderGraph *render_graph, ImageProxyId image_proxy_id)
{
	this->render_graph_   = render_graph;
	this->image_proxy_id_ = image_proxy_id;
}

void RenderGraph::ImageHandleInfo::reset()
{
	render_graph_->delete_image(image_proxy_id_);
}

RenderGraph::ImageProxyId RenderGraph::ImageHandleInfo::id() const
{
	return image_proxy_id_;
}

void RenderGraph::ImageHandleInfo::set_debug_name(std::string name) const
{
	render_graph_->set_image_proxy_debug_name(image_proxy_id_, name);
}

RenderGraph::ImageViewHandleInfo::ImageViewHandleInfo()
{
}

RenderGraph::ImageViewHandleInfo::ImageViewHandleInfo(RenderGraph     *render_graph,
                                                      ImageViewProxyId image_view_proxy_id)
{
	this->render_graph_        = render_graph;
	this->image_view_proxy_id_ = image_view_proxy_id;
}

void RenderGraph::ImageViewHandleInfo::reset()
{
	render_graph_->delete_image_view(image_view_proxy_id_);
}

RenderGraph::ImageViewProxyId RenderGraph::ImageViewHandleInfo::id() const
{
	return image_view_proxy_id_;
}

void RenderGraph::ImageViewHandleInfo::set_debug_name(std::string name) const
{
	render_graph_->set_image_view_proxy_debug_name(image_view_proxy_id_, name);
}

RenderGraph::BufferHandleInfo::BufferHandleInfo()
{
}

RenderGraph::BufferHandleInfo::BufferHandleInfo(RenderGraph *render_graph, BufferProxyId buffer_proxy_id)
{
	this->render_graph_    = render_graph;
	this->buffer_proxy_id_ = buffer_proxy_id;
}

void RenderGraph::BufferHandleInfo::reset()
{
	render_graph_->delete_buffer(buffer_proxy_id_);
}

RenderGraph::BufferProxyId RenderGraph::BufferHandleInfo::id() const
{
	return buffer_proxy_id_;
}

RenderGraph::RenderGraph(vk::PhysicalDevice physical_device, vk::Device logical_device,
                         vk::DispatchLoaderDynamic loader) :
    physical_device_(physical_device),
    logical_device_(logical_device),
    loader_(loader),
    render_pass_cache_(logical_device),
    framebuffer_cache_(logical_device),
    image_cache_(physical_device, logical_device, loader),
    image_view_cache_(physical_device, logical_device),
    buffer_cache_(physical_device, logical_device)
{
}

RenderGraph::ImageProxyUnique RenderGraph::add_image(vk::Format format, uint32_t mips_count,
                                                     uint32_t array_layers_count, glm::uvec2 size,
                                                     vk::ImageUsageFlags usage_flags)
{
	return add_image(format, mips_count, array_layers_count, glm::uvec3(size.x, size.y, -1), usage_flags);
}

RenderGraph::ImageProxyUnique RenderGraph::add_image(vk::Format format, uint32_t mips_count,
                                                     uint32_t array_layers_count, glm::uvec3 size,
                                                     vk::ImageUsageFlags usage_flags)
{
	ImageProxy image_proxy;
	image_proxy.type                         = ImageProxy::Types::eTransient;
	image_proxy.image_key.format             = format;
	image_proxy.image_key.usage_flags        = usage_flags;
	image_proxy.image_key.mips_count         = mips_count;
	image_proxy.image_key.array_layers_count = array_layers_count;
	image_proxy.image_key.size               = size;

	image_proxy.external_image            = nullptr;
	auto              unique_proxy_handle = ImageProxyUnique(ImageHandleInfo(this, image_proxies_.add(std::move(image_proxy))));
	const std::string debug_name          = std::string("Graph image [") + std::to_string(size.x) + ", " +
	                               std::to_string(size.y) + ", Id=" + std::to_string(unique_proxy_handle->id().as_int) + "]" + vk::to_string(image_proxy.image_key.format);
	unique_proxy_handle->set_debug_name(debug_name);
	return unique_proxy_handle;
}

RenderGraph::ImageProxyUnique RenderGraph::add_external_image(lz::ImageData *image)
{
	ImageProxy image_proxy;
	image_proxy.type                 = ImageProxy::Types::eExternal;
	image_proxy.external_image       = image;
	image_proxy.image_key.debug_name = "External graph image";
	return ImageProxyUnique(ImageHandleInfo(this, image_proxies_.add(std::move(image_proxy))));
}

RenderGraph::ImageViewProxyUnique RenderGraph::add_image_view(ImageProxyId image_proxy_id, uint32_t base_mip_level,
                                                              uint32_t mip_levels_count, uint32_t base_array_layer,
                                                              uint32_t array_layers_count)
{
	ImageViewProxy image_view_proxy;
	image_view_proxy.external_view                        = nullptr;
	image_view_proxy.type                                 = ImageViewProxy::Types::eTransient;
	image_view_proxy.image_proxy_id                       = image_proxy_id;
	image_view_proxy.subresource_range.base_mip_level     = base_mip_level;
	image_view_proxy.subresource_range.mips_count         = mip_levels_count;
	image_view_proxy.subresource_range.base_array_layer   = base_array_layer;
	image_view_proxy.subresource_range.array_layers_count = array_layers_count;
	image_view_proxy.debug_name                           = "View";
	return ImageViewProxyUnique(ImageViewHandleInfo(this, image_view_proxies_.add(std::move(image_view_proxy))));
}

RenderGraph::ImageViewProxyUnique RenderGraph::add_external_image_view(lz::ImageView      *image_view,
                                                                       lz::ImageUsageTypes usage_type)
{
	ImageViewProxy image_view_proxy;
	image_view_proxy.external_view       = image_view;
	image_view_proxy.external_usage_type = usage_type;
	image_view_proxy.type                = ImageViewProxy::Types::eExternal;
	image_view_proxy.image_proxy_id      = ImageProxyId();
	image_view_proxy.debug_name          = "External view";
	return ImageViewProxyUnique(ImageViewHandleInfo(this, image_view_proxies_.add(std::move(image_view_proxy))));
}

void RenderGraph::delete_image(ImageProxyId image_id)
{
	image_proxies_.release(image_id);
}

void RenderGraph::set_image_proxy_debug_name(ImageProxyId image_id, std::string debug_name)
{
	image_proxies_.get(image_id).image_key.debug_name = debug_name;
}

void RenderGraph::set_image_view_proxy_debug_name(ImageViewProxyId image_view_id, std::string debug_name)
{
	image_view_proxies_.get(image_view_id).debug_name = debug_name;
}

void RenderGraph::delete_image_view(ImageViewProxyId image_view_id)
{
	image_view_proxies_.release(image_view_id);
}

void RenderGraph::delete_buffer(BufferProxyId buffer_id)
{
	buffer_proxies_.release(buffer_id);
}

glm::uvec2 RenderGraph::get_mip_size(ImageProxyId image_proxy_id, uint32_t mip_level)
{
	const auto &image_proxy = image_proxies_.get(image_proxy_id);
	switch (image_proxy.type)
	{
		case ImageProxy::Types::eExternal:
		{
			return image_proxy.external_image->get_mip_size(mip_level);
		}
		break;
		case ImageProxy::Types::eTransient:
		{
			const glm::u32 mip_mult = (1 << mip_level);
			return glm::uvec2(image_proxy.image_key.size.x / mip_mult, image_proxy.image_key.size.y / mip_mult);
		}
		break;
	}
	return glm::uvec2(-1, -1);
}

glm::uvec2 RenderGraph::get_mip_size(ImageViewProxyId image_view_proxy_id, uint32_t mip_offset)
{
	const auto &image_view_proxy = image_view_proxies_.get(image_view_proxy_id);
	switch (image_view_proxy.type)
	{
		case ImageViewProxy::Types::eExternal:
		{
			const uint32_t mip_level = image_view_proxy.external_view->get_base_mip_level() + mip_offset;
			return image_view_proxy.external_view->get_image_data()->get_mip_size(mip_level);
		}
		break;
		case ImageViewProxy::Types::eTransient:
		{
			const uint32_t mip_level = image_view_proxy.subresource_range.base_mip_level + mip_offset;
			return get_mip_size(image_view_proxy.image_proxy_id, mip_level);
		}
		break;
	}
	return glm::uvec2(-1, -1);
}

RenderGraph::BufferProxyUnique RenderGraph::add_external_buffer(lz::Buffer *buffer)
{
	BufferProxy buffer_proxy;
	buffer_proxy.type                      = BufferProxy::Types::eExternal;
	buffer_proxy.buffer_key.element_size   = -1;
	buffer_proxy.buffer_key.elements_count = -1;
	buffer_proxy.external_buffer           = buffer;
	return BufferProxyUnique(BufferHandleInfo(this, buffer_proxies_.add(std::move(buffer_proxy))));
}

lz::ImageView *RenderGraph::PassContext::get_image_view(ImageViewProxyId image_view_proxy_id) const
{
	return resolved_image_views_[image_view_proxy_id.as_int];
}

lz::Buffer *RenderGraph::PassContext::get_buffer(BufferProxyId buffer_proxy) const
{
	return resolved_buffers_[buffer_proxy.as_int];
}

vk::CommandBuffer RenderGraph::PassContext::get_command_buffer() const
{
	return command_buffer_;
}

lz::RenderPass *RenderGraph::RenderPassContext::get_render_pass() const
{
	return render_pass_;
}

RenderGraph::RenderPassDesc::RenderPassDesc()
{
	profiler_task_name  = "RenderPass";
	profiler_task_color = glm::packUnorm4x8(glm::vec4(1.0f, 0.5f, 0.0f, 1.0f));
}

RenderGraph::RenderPassDesc &RenderGraph::RenderPassDesc::set_color_attachments(
    const std::vector<ImageViewProxyId> &color_attachment_view_proxies, vk::AttachmentLoadOp load_op,
    vk::ClearValue clear_value)
{
	this->color_attachments.resize(color_attachment_view_proxies.size());
	for (size_t index = 0; index < color_attachment_view_proxies.size(); ++index)
	{
		this->color_attachments[index] = {color_attachment_view_proxies[index], load_op, clear_value};
	}
	return *this;
}

RenderGraph::RenderPassDesc &RenderGraph::RenderPassDesc::set_color_attachments(
    std::vector<Attachment> &&color_attachments)
{
	this->color_attachments = std::move(color_attachments);
	return *this;
}

RenderGraph::RenderPassDesc &RenderGraph::RenderPassDesc::set_depth_attachment(
    ImageViewProxyId depth_attachment_view_proxy_id, vk::AttachmentLoadOp load_op, vk::ClearValue _clearValue)
{
	this->depth_attachment.image_view_proxy_id = depth_attachment_view_proxy_id;
	this->depth_attachment.load_op             = load_op;
	this->depth_attachment.clear_value         = _clearValue;
	return *this;
}

RenderGraph::RenderPassDesc &RenderGraph::RenderPassDesc::set_depth_attachment(Attachment depth_attachment)
{
	this->depth_attachment = depth_attachment;
	return *this;
}

RenderGraph::RenderPassDesc &RenderGraph::RenderPassDesc::set_vertex_buffers(
    std::vector<BufferProxyId> &&vertex_buffer_proxies)
{
	this->vertex_buffer_proxies = std::move(vertex_buffer_proxies);
	return *this;
}

RenderGraph::RenderPassDesc &RenderGraph::RenderPassDesc::set_input_images(
    std::vector<ImageViewProxyId> &&input_image_view_proxies)
{
	this->input_image_view_proxies = std::move(input_image_view_proxies);
	return *this;
}

RenderGraph::RenderPassDesc &RenderGraph::RenderPassDesc::set_storage_buffers(
    std::vector<BufferProxyId> &&inout_storage_buffer_proxies)
{
	this->inout_storage_buffer_proxies = inout_storage_buffer_proxies;
	return *this;
}

RenderGraph::RenderPassDesc &RenderGraph::RenderPassDesc::set_storage_images(
    std::vector<ImageViewProxyId> &&inout_storage_image_proxies)
{
	this->inout_storage_image_proxies = inout_storage_image_proxies;
	return *this;
}

RenderGraph::RenderPassDesc &RenderGraph::RenderPassDesc::set_render_area_extent(vk::Extent2D render_area_extent)
{
	this->render_area_extent = render_area_extent;
	return *this;
}

RenderGraph::RenderPassDesc &RenderGraph::RenderPassDesc::set_record_func(
    std::function<void(RenderPassContext)> record_func)
{
	this->record_func = record_func;
	return *this;
}

RenderGraph::RenderPassDesc &RenderGraph::RenderPassDesc::set_profiler_info(
    uint32_t task_color, std::string task_name)
{
	this->profiler_task_color = task_color;
	this->profiler_task_name  = task_name;
	return *this;
}

void RenderGraph::add_render_pass(std::vector<ImageViewProxyId> color_attachment_image_proxies,
                                  ImageViewProxyId              depth_attachment_image_proxy,
                                  std::vector<ImageViewProxyId> input_image_view_proxies,
                                  vk::Extent2D render_area_extent, vk::AttachmentLoadOp load_op,
                                  std::function<void(RenderPassContext)> record_func)
{
	RenderPassDesc render_pass_desc;
	for (const auto &proxy : color_attachment_image_proxies)
	{
		RenderPassDesc::Attachment color_attachment = {
		    proxy, load_op, vk::ClearColorValue(std::array<float, 4>{0.03f, 0.03f, 0.03f, 1.0f})};
		render_pass_desc.color_attachments.push_back(color_attachment);
	}
	render_pass_desc.depth_attachment = {
	    depth_attachment_image_proxy, load_op, vk::ClearDepthStencilValue(1.0f, 0)};
	render_pass_desc.input_image_view_proxies     = input_image_view_proxies;
	render_pass_desc.inout_storage_buffer_proxies = {};
	render_pass_desc.render_area_extent           = render_area_extent;
	render_pass_desc.record_func                  = record_func;

	add_pass(render_pass_desc);
}

void RenderGraph::add_pass(RenderPassDesc &render_pass_desc)
{
	Task task;
	task.type  = Task::Types::eRenderPass;
	task.index = render_pass_descs_.size();
	add_task(task);

	render_pass_descs_.emplace_back(render_pass_desc);
}

void RenderGraph::clear()
{
	*this = RenderGraph(physical_device_, logical_device_, loader_);
}

RenderGraph::ComputePassDesc::ComputePassDesc()
{
	profiler_task_name  = "ComputePass";
	profiler_task_color = lz::Colors::belize_hole;
}

RenderGraph::ComputePassDesc &RenderGraph::ComputePassDesc::set_input_images(
    std::vector<ImageViewProxyId> &&input_image_view_proxies)
{
	this->input_image_view_proxies = std::move(input_image_view_proxies);
	return *this;
}

RenderGraph::ComputePassDesc &RenderGraph::ComputePassDesc::set_storage_buffers(
    std::vector<BufferProxyId> &&inout_storage_buffer_proxies)
{
	this->inout_storage_buffer_proxies = inout_storage_buffer_proxies;
	return *this;
}

RenderGraph::ComputePassDesc &RenderGraph::ComputePassDesc::set_storage_images(
    std::vector<ImageViewProxyId> &&inout_storage_image_proxies)
{
	this->inout_storage_image_proxies = inout_storage_image_proxies;
	return *this;
}

RenderGraph::ComputePassDesc &RenderGraph::ComputePassDesc::set_record_func(
    std::function<void(PassContext)> record_func)
{
	this->record_func = record_func;
	return *this;
}

RenderGraph::ComputePassDesc &RenderGraph::ComputePassDesc::set_profiler_info(uint32_t    task_color,
                                                                              std::string task_name)
{
	this->profiler_task_color = task_color;
	this->profiler_task_name  = task_name;
	return *this;
}

void RenderGraph::add_compute_pass(std::vector<BufferProxyId>       inout_buffer_proxies,
                                   std::vector<ImageViewProxyId>    input_image_view_proxies,
                                   std::function<void(PassContext)> record_func)
{
	ComputePassDesc compute_pass_desc;
	compute_pass_desc.inout_storage_buffer_proxies = inout_buffer_proxies;
	compute_pass_desc.input_image_view_proxies     = input_image_view_proxies;
	compute_pass_desc.record_func                  = record_func;

	add_pass(compute_pass_desc);
}

void RenderGraph::add_pass(ComputePassDesc &compute_pass_desc)
{
	Task task;
	task.type  = Task::Types::eComputePass;
	task.index = compute_pass_descs_.size();
	add_task(task);

	compute_pass_descs_.emplace_back(compute_pass_desc);
}

RenderGraph::TransferPassDesc::TransferPassDesc()
{
	profiler_task_name  = "TransferPass";
	profiler_task_color = lz::Colors::silver;
}

RenderGraph::TransferPassDesc &RenderGraph::TransferPassDesc::set_src_images(
    std::vector<ImageViewProxyId> &&src_image_view_proxies)
{
	this->src_image_view_proxies = std::move(src_image_view_proxies);
	return *this;
}

RenderGraph::TransferPassDesc &RenderGraph::TransferPassDesc::set_dst_images(
    std::vector<ImageViewProxyId> &&dst_image_view_proxies)
{
	this->dst_image_view_proxies = std::move(dst_image_view_proxies);
	return *this;
}

RenderGraph::TransferPassDesc &RenderGraph::TransferPassDesc::set_src_buffers(
    std::vector<BufferProxyId> &&src_buffer_proxies)
{
	this->src_buffer_proxies = std::move(src_buffer_proxies);
	return *this;
}

RenderGraph::TransferPassDesc &RenderGraph::TransferPassDesc::set_dst_buffers(
    std::vector<BufferProxyId> &&dst_buffer_proxies)
{
	this->dst_buffer_proxies = std::move(dst_buffer_proxies);
	return *this;
}

RenderGraph::TransferPassDesc &RenderGraph::TransferPassDesc::set_record_func(
    std::function<void(PassContext)> record_func)
{
	this->record_func = record_func;
	return *this;
}

RenderGraph::TransferPassDesc &RenderGraph::TransferPassDesc::set_profiler_info(uint32_t    task_color,
                                                                                std::string task_name)
{
	this->profiler_task_color = task_color;
	this->profiler_task_name  = task_name;
	return *this;
}

void RenderGraph::add_pass(TransferPassDesc &transfer_pass_desc)
{
	Task task;
	task.type  = Task::Types::eTransferPass;
	task.index = transfer_pass_descs_.size();
	add_task(task);

	transfer_pass_descs_.emplace_back(transfer_pass_desc);
}

RenderGraph::ImagePresentPassDesc &RenderGraph::ImagePresentPassDesc::set_image(
    ImageViewProxyId present_image_view_proxy_id)
{
	this->present_image_view_proxy_id = present_image_view_proxy_id;
	return *this;
}

void RenderGraph::add_pass(ImagePresentPassDesc &&image_present_desc)
{
	Task task;
	task.type  = Task::Types::eImagePresent;
	task.index = image_present_descs_.size();
	add_task(task);

	image_present_descs_.push_back(image_present_desc);
}

void RenderGraph::add_image_present(ImageViewProxyId present_image_view_proxy_id)
{
	ImagePresentPassDesc image_present_desc;
	image_present_desc.present_image_view_proxy_id = present_image_view_proxy_id;

	add_pass(std::move(image_present_desc));
}

void RenderGraph::add_pass(FrameSyncBeginPassDesc &&frame_sync_begin_desc)
{
	Task task;
	task.type  = Task::Types::eFrameSyncBegin;
	task.index = frame_sync_begin_descs_.size();
	add_task(task);

	frame_sync_begin_descs_.push_back(frame_sync_begin_desc);
}

void RenderGraph::add_pass(FrameSyncEndPassDesc &&frame_sync_end_desc)
{
	Task task;
	task.type  = Task::Types::eFrameSyncEnd;
	task.index = frame_sync_end_descs_.size();
	add_task(task);

	frame_sync_end_descs_.push_back(frame_sync_end_desc);
}

void RenderGraph::execute(vk::CommandBuffer command_buffer, lz::CpuProfiler *cpu_profiler,
                          lz::GpuProfiler *gpu_profiler)
{
	resolve_images();
	resolve_image_views();
	resolve_buffers();

	for (size_t task_index = 0; task_index < tasks_.size(); ++task_index)
	{
		auto &task = tasks_[task_index];
		switch (task.type)
		{
			case Task::Types::eRenderPass:
			{
				auto &render_pass_desc = render_pass_descs_[task.index];
				auto  profiler_task    = create_profiler_task(render_pass_desc);
				auto  gpu_task         = gpu_profiler->start_scoped_task(profiler_task.name, profiler_task.color,
				                                                         vk::PipelineStageFlagBits::eBottomOfPipe);
				auto  cpu_task         = cpu_profiler->start_scoped_task(profiler_task.name, profiler_task.color);

				RenderPassContext pass_context;
				pass_context.resolved_image_views_.resize(image_view_proxies_.get_size(), nullptr);
				pass_context.resolved_buffers_.resize(buffer_proxies_.get_size(), nullptr);

				for (auto &input_image_view_proxy : render_pass_desc.input_image_view_proxies)
				{
					pass_context.resolved_image_views_[input_image_view_proxy.as_int] = get_resolved_image_view(
					    task_index, input_image_view_proxy);
				}

				for (auto &inout_storage_image_proxy : render_pass_desc.inout_storage_image_proxies)
				{
					pass_context.resolved_image_views_[inout_storage_image_proxy.as_int] = get_resolved_image_view(
					    task_index, inout_storage_image_proxy);
				}

				for (auto &inout_buffer_proxy : render_pass_desc.inout_storage_buffer_proxies)
				{
					pass_context.resolved_buffers_[inout_buffer_proxy.as_int] = get_resolved_buffer(
					    task_index, inout_buffer_proxy);
				}

				for (auto &vertex_buffer_proxy : render_pass_desc.vertex_buffer_proxies)
				{
					pass_context.resolved_buffers_[vertex_buffer_proxy.as_int] = get_resolved_buffer(
					    task_index, vertex_buffer_proxy);
				}

				vk::PipelineStageFlags              src_stage;
				vk::PipelineStageFlags              dst_stage;
				std::vector<vk::ImageMemoryBarrier> image_barriers;

				for (auto input_image_view_proxy : render_pass_desc.input_image_view_proxies)
				{
					auto image_view = get_resolved_image_view(task_index, input_image_view_proxy);
					add_image_transition_barriers(image_view, ImageUsageTypes::eGraphicsShaderRead, task_index,
					                              src_stage, dst_stage, image_barriers);
				}

				for (auto &inout_storage_image_proxy : render_pass_desc.inout_storage_image_proxies)
				{
					auto image_view = get_resolved_image_view(task_index, inout_storage_image_proxy);
					add_image_transition_barriers(image_view, ImageUsageTypes::eGraphicsShaderReadWrite, task_index,
					                              src_stage, dst_stage, image_barriers);
				}

				for (auto &color_attachment : render_pass_desc.color_attachments)
				{
					auto image_view = get_resolved_image_view(task_index, color_attachment.image_view_proxy_id);
					add_image_transition_barriers(image_view, ImageUsageTypes::eColorAttachment, task_index,
					                              src_stage,
					                              dst_stage, image_barriers);
				}

				if (!(render_pass_desc.depth_attachment.image_view_proxy_id == ImageViewProxyId()))
				{
					auto image_view = get_resolved_image_view(
					    task_index, render_pass_desc.depth_attachment.image_view_proxy_id);
					add_image_transition_barriers(image_view, ImageUsageTypes::eDepthAttachment, task_index,
					                              src_stage,
					                              dst_stage, image_barriers);
				}

				std::vector<vk::BufferMemoryBarrier> buffer_barriers;

				for (auto vertex_buffer_proxy : render_pass_desc.vertex_buffer_proxies)
				{
					auto storage_buffer = get_resolved_buffer(task_index, vertex_buffer_proxy);
					add_buffer_barriers(storage_buffer, BufferUsageTypes::eVertexBuffer, task_index, src_stage,
					                    dst_stage, buffer_barriers);
				}

				for (auto inout_buffer_proxy : render_pass_desc.inout_storage_buffer_proxies)
				{
					auto storage_buffer = get_resolved_buffer(task_index, inout_buffer_proxy);
					add_buffer_barriers(storage_buffer, BufferUsageTypes::eGraphicsShaderReadWrite, task_index,
					                    src_stage, dst_stage, buffer_barriers);
				}

				if (image_barriers.size() > 0 || buffer_barriers.size() > 0)
				{
					command_buffer.pipelineBarrier(src_stage, dst_stage, vk::DependencyFlags(), {}, buffer_barriers,
					                               image_barriers);
				}

				std::vector<FramebufferCache::Attachment> color_attachments;
				FramebufferCache::Attachment              depth_attachment;

				lz::RenderPassCache::RenderPassKey render_pass_key;

				for (auto &attachment : render_pass_desc.color_attachments)
				{
					auto image_view = get_resolved_image_view(task_index, attachment.image_view_proxy_id);

					render_pass_key.color_attachment_descs.push_back({image_view->get_image_data()->get_format(), attachment.load_op, attachment.clear_value});
					color_attachments.push_back({image_view, attachment.clear_value});
				}
				bool depth_present = !(render_pass_desc.depth_attachment.image_view_proxy_id == ImageViewProxyId());
				if (depth_present)
				{
					auto image_view = get_resolved_image_view(
					    task_index, render_pass_desc.depth_attachment.image_view_proxy_id);

					render_pass_key.depth_attachment_desc = {
					    image_view->get_image_data()->get_format(), render_pass_desc.depth_attachment.load_op,
					    render_pass_desc.depth_attachment.clear_value};
					depth_attachment = {image_view, render_pass_desc.depth_attachment.clear_value};
				}
				else
				{
					render_pass_key.depth_attachment_desc.format = vk::Format::eUndefined;
				}

				auto render_pass          = render_pass_cache_.get_render_pass(render_pass_key);
				pass_context.render_pass_ = render_pass;

				framebuffer_cache_.begin_pass(command_buffer, color_attachments,
				                              depth_present ? (&depth_attachment) : nullptr, render_pass,
				                              render_pass_desc.render_area_extent);
				pass_context.command_buffer_ = command_buffer;
				render_pass_desc.record_func(pass_context);
				framebuffer_cache_.end_pass(command_buffer);
			}
			break;
			case Task::Types::eComputePass:
			{
				auto &compute_pass_desc = compute_pass_descs_[task.index];
				auto  profiler_task     = create_profiler_task(compute_pass_desc);
				auto  gpu_task          = gpu_profiler->start_scoped_task(profiler_task.name, profiler_task.color,
				                                                          vk::PipelineStageFlagBits::eBottomOfPipe);
				auto  cpu_task          = cpu_profiler->start_scoped_task(profiler_task.name, profiler_task.color);

				PassContext pass_context;
				pass_context.resolved_image_views_.resize(image_view_proxies_.get_size(), nullptr);
				pass_context.resolved_buffers_.resize(buffer_proxies_.get_size(), nullptr);

				for (auto &input_image_view_proxy : compute_pass_desc.input_image_view_proxies)
				{
					pass_context.resolved_image_views_[input_image_view_proxy.as_int] = get_resolved_image_view(
					    task_index, input_image_view_proxy);
				}

				for (auto &inout_buffer_proxy : compute_pass_desc.inout_storage_buffer_proxies)
				{
					pass_context.resolved_buffers_[inout_buffer_proxy.as_int] = get_resolved_buffer(
					    task_index, inout_buffer_proxy);
				}

				for (auto &inout_storage_image_proxy : compute_pass_desc.inout_storage_image_proxies)
				{
					pass_context.resolved_image_views_[inout_storage_image_proxy.as_int] = get_resolved_image_view(
					    task_index, inout_storage_image_proxy);
				}

				vk::PipelineStageFlags src_stage;
				vk::PipelineStageFlags dst_stage;

				std::vector<vk::ImageMemoryBarrier> image_barriers;
				for (auto input_image_view_proxy : compute_pass_desc.input_image_view_proxies)
				{
					auto image_view = get_resolved_image_view(task_index, input_image_view_proxy);
					add_image_transition_barriers(image_view, ImageUsageTypes::eComputeShaderRead, task_index,
					                              src_stage, dst_stage, image_barriers);
				}

				for (auto &inout_storage_image_proxy : compute_pass_desc.inout_storage_image_proxies)
				{
					auto image_view = get_resolved_image_view(task_index, inout_storage_image_proxy);
					add_image_transition_barriers(image_view, ImageUsageTypes::eComputeShaderReadWrite, task_index,
					                              src_stage, dst_stage, image_barriers);
				}

				std::vector<vk::BufferMemoryBarrier> buffer_barriers;
				for (auto inout_buffer_proxy : compute_pass_desc.inout_storage_buffer_proxies)
				{
					auto storage_buffer = get_resolved_buffer(task_index, inout_buffer_proxy);
					add_buffer_barriers(storage_buffer, BufferUsageTypes::eComputeShaderReadWrite, task_index,
					                    src_stage, dst_stage, buffer_barriers);
				}

				if (image_barriers.size() > 0 || buffer_barriers.size() > 0)
				{
					command_buffer.pipelineBarrier(src_stage, dst_stage, vk::DependencyFlags(), {}, buffer_barriers,
					                               image_barriers);
				}

				pass_context.command_buffer_ = command_buffer;
				if (compute_pass_desc.record_func)
				{
					compute_pass_desc.record_func(pass_context);
				}
			}
			break;
			case Task::Types::eTransferPass:
			{
				auto &transfer_pass_desc = transfer_pass_descs_[task.index];
				auto  profiler_task      = create_profiler_task(transfer_pass_desc);
				auto  gpu_task           = gpu_profiler->start_scoped_task(profiler_task.name, profiler_task.color,
				                                                           vk::PipelineStageFlagBits::eBottomOfPipe);
				auto  cpu_task           = cpu_profiler->start_scoped_task(profiler_task.name, profiler_task.color);

				PassContext pass_context;
				pass_context.resolved_image_views_.resize(image_view_proxies_.get_size(), nullptr);
				pass_context.resolved_buffers_.resize(buffer_proxies_.get_size(), nullptr);

				for (auto &src_image_view_proxy : transfer_pass_desc.src_image_view_proxies)
				{
					pass_context.resolved_image_views_[src_image_view_proxy.as_int] = get_resolved_image_view(
					    task_index, src_image_view_proxy);
				}
				for (auto &dst_image_view_proxy : transfer_pass_desc.dst_image_view_proxies)
				{
					pass_context.resolved_image_views_[dst_image_view_proxy.as_int] = get_resolved_image_view(
					    task_index, dst_image_view_proxy);
				}

				for (auto &src_buffer_proxy : transfer_pass_desc.src_buffer_proxies)
				{
					pass_context.resolved_buffers_[src_buffer_proxy.as_int] = get_resolved_buffer(
					    task_index, src_buffer_proxy);
				}

				for (auto &dst_buffer_proxy : transfer_pass_desc.dst_buffer_proxies)
				{
					pass_context.resolved_buffers_[dst_buffer_proxy.as_int] = get_resolved_buffer(
					    task_index, dst_buffer_proxy);
				}

				vk::PipelineStageFlags src_stage;
				vk::PipelineStageFlags dst_stage;

				std::vector<vk::ImageMemoryBarrier> image_barriers;
				for (auto src_image_view_proxy : transfer_pass_desc.src_image_view_proxies)
				{
					auto image_view = get_resolved_image_view(task_index, src_image_view_proxy);
					add_image_transition_barriers(image_view, ImageUsageTypes::eTransferSrc, task_index, src_stage,
					                              dst_stage, image_barriers);
				}

				for (auto dst_image_view_proxy : transfer_pass_desc.dst_image_view_proxies)
				{
					auto image_view = get_resolved_image_view(task_index, dst_image_view_proxy);
					add_image_transition_barriers(image_view, ImageUsageTypes::eTransferDst, task_index, src_stage,
					                              dst_stage, image_barriers);
				}

				std::vector<vk::BufferMemoryBarrier> buffer_barriers;
				for (auto src_buffer_proxy : transfer_pass_desc.src_buffer_proxies)
				{
					auto storage_buffer = get_resolved_buffer(task_index, src_buffer_proxy);
					add_buffer_barriers(storage_buffer, BufferUsageTypes::eTransferSrc, task_index, src_stage,
					                    dst_stage, buffer_barriers);
				}

				for (auto dst_buffer_proxy : transfer_pass_desc.dst_buffer_proxies)
				{
					auto storage_buffer = get_resolved_buffer(task_index, dst_buffer_proxy);
					add_buffer_barriers(storage_buffer, BufferUsageTypes::eTransferSrc, task_index, src_stage,
					                    dst_stage, buffer_barriers);
				}

				if (image_barriers.size() > 0 || buffer_barriers.size() > 0)
					command_buffer.pipelineBarrier(src_stage, dst_stage, vk::DependencyFlags(), {}, buffer_barriers,
					                               image_barriers);

				pass_context.command_buffer_ = command_buffer;
				if (transfer_pass_desc.record_func)
					transfer_pass_desc.record_func(pass_context);
			}
			break;
			case Task::Types::eImagePresent:
			{
				auto &image_present_desc = image_present_descs_[task.index];
				auto  profiler_task      = create_profiler_task(image_present_desc);
				auto  gpu_task           = gpu_profiler->start_scoped_task(profiler_task.name, profiler_task.color,
				                                                           vk::PipelineStageFlagBits::eBottomOfPipe);
				auto  cpu_task           = cpu_profiler->start_scoped_task(profiler_task.name, profiler_task.color);

				vk::PipelineStageFlags              src_stage;
				vk::PipelineStageFlags              dst_stage;
				std::vector<vk::ImageMemoryBarrier> image_barriers;
				{
					auto image_view =
					    get_resolved_image_view(task_index, image_present_desc.present_image_view_proxy_id);
					add_image_transition_barriers(image_view, ImageUsageTypes::ePresent, task_index, src_stage,
					                              dst_stage, image_barriers);
				}

				if (image_barriers.size() > 0)
					command_buffer.pipelineBarrier(src_stage, dst_stage, vk::DependencyFlags(), {}, {},
					                               image_barriers);
			}
			break;
			case Task::Types::eFrameSyncBegin:
			{
				auto &frame_sync_desc = frame_sync_begin_descs_[task.index];
				auto  profiler_task   = create_profiler_task(frame_sync_desc);
				auto  gpu_task        = gpu_profiler->start_scoped_task(profiler_task.name, profiler_task.color,
				                                                        vk::PipelineStageFlagBits::eBottomOfPipe);
				auto  cpu_task        = cpu_profiler->start_scoped_task(profiler_task.name, profiler_task.color);

				std::vector<vk::ImageMemoryBarrier> image_barriers;
				vk::PipelineStageFlags              src_stage = vk::PipelineStageFlagBits::eBottomOfPipe;
				vk::PipelineStageFlags              dst_stage = vk::PipelineStageFlagBits::eTopOfPipe;

				auto memory_barrier = vk::MemoryBarrier();
				command_buffer.pipelineBarrier(src_stage, dst_stage, vk::DependencyFlags(), {memory_barrier}, {},
				                               {});
			}
			break;
			case Task::Types::eFrameSyncEnd:
			{
				auto &frame_sync_desc = frame_sync_end_descs_[task.index];
				auto  profiler_task   = create_profiler_task(frame_sync_desc);
				auto  gpu_task        = gpu_profiler->start_scoped_task(profiler_task.name, profiler_task.color,
				                                                        vk::PipelineStageFlagBits::eBottomOfPipe);
				auto  cpu_task        = cpu_profiler->start_scoped_task(profiler_task.name, profiler_task.color);

				std::vector<vk::ImageMemoryBarrier> image_barriers;
				vk::PipelineStageFlags              src_stage = vk::PipelineStageFlagBits::eBottomOfPipe;
				vk::PipelineStageFlags              dst_stage = vk::PipelineStageFlagBits::eTopOfPipe;
				for (auto image_view_proxy : image_view_proxies_)
				{
					if (image_view_proxy.external_view != nullptr && image_view_proxy.external_usage_type != lz::ImageUsageTypes::eUnknown && image_view_proxy.external_usage_type != lz::ImageUsageTypes::eNone)
						add_image_transition_barriers(image_view_proxy.external_view,
						                              image_view_proxy.external_usage_type, task_index, src_stage,
						                              dst_stage, image_barriers);
				}

				/*std::vector<vk::BufferMemoryBarrier> bufferBarriers;
				    for (auto bufferProxy : buffer_proxies_)
				    {
				      if(bufferProxy.externalBuffer != nullptr)
				      auto storageBuffer = GetResolvedBuffer(taskIndex, inoutBufferProxy);
				      AddBufferBarriers(storageBuffer, BufferUsageTypes::ComputeShaderReadWrite, taskIndex, srcStage, dstStage, bufferBarriers);
				    }*/

				if (image_barriers.size() > 0)
					command_buffer.pipelineBarrier(src_stage, dst_stage, vk::DependencyFlags(), {}, {},
					                               image_barriers);
			}
			break;
		}
	}

	flush_external_images(command_buffer, cpu_profiler, gpu_profiler);

	render_pass_descs_.clear();
	transfer_pass_descs_.clear();
	image_present_descs_.clear();
	frame_sync_begin_descs_.clear();
	frame_sync_end_descs_.clear();
	tasks_.clear();
}

void RenderGraph::flush_external_images(vk::CommandBuffer command_buffer_, lz::CpuProfiler *cpu_profiler,
                                        lz::GpuProfiler *gpu_profiler)
{
	// need to make sure I don't override the same mip level of an image twice if there's more than view for it

	/*if (tasks_.size() == 0) return;
	    size_t lastTaskIndex = tasks_.size() - 1;

	    for (auto &imageViewProxy : this->imageViewProxies)
	    {
	      if (imageViewProxy.type == ImageViewProxy::Types::External)
	      {
	        vk::PipelineStageFlags srcStage;
	        vk::PipelineStageFlags dstStage;

	        std::vector<vk::ImageMemoryBarrier> imageBarriers;
	        if (imageViewProxy.externalUsageType != lz::ImageUsageTypes::Unknown)
	        {
	          AddImageTransitionBarriers(imageViewProxy.externalView, imageViewProxy.externalUsageType, tasks_.size(), srcStage, dstStage, imageBarriers);
	        }

	        if (imageBarriers.size() > 0)
	          command_buffer_.pipelineBarrier(srcStage, dstStage, vk::DependencyFlags(), {}, {}, imageBarriers);
	      }
	    }*/
}

bool RenderGraph::image_view_contains_subresource(lz::ImageView *image_view, lz::ImageData *image_data,
                                                  uint32_t mip_level, uint32_t array_layer)
{
	return (
	    image_view->get_image_data() == image_data &&
	    array_layer >= image_view->get_base_array_layer() && array_layer < image_view->get_base_array_layer() + image_view->get_array_layers_count() &&
	    mip_level >= image_view->get_base_mip_level() && mip_level < image_view->get_base_mip_level() + image_view->get_mip_levels_count());
}

ImageUsageTypes RenderGraph::get_task_image_subresource_usage_type(size_t task_index, lz::ImageData *image_data,
                                                                   uint32_t mip_level, uint32_t array_layer)
{
	Task &task = tasks_[task_index];
	switch (task.type)
	{
		case Task::Types::eRenderPass:
		{
			const auto &render_pass_desc = render_pass_descs_[task.index];
			for (const auto &color_attachment : render_pass_desc.color_attachments)
			{
				const auto attachment_image_view = get_resolved_image_view(
				    task_index, color_attachment.image_view_proxy_id);
				if (image_view_contains_subresource(attachment_image_view, image_data, mip_level, array_layer))
				{
					return ImageUsageTypes::eColorAttachment;
				}
			}

			if (!(render_pass_desc.depth_attachment.image_view_proxy_id == ImageViewProxyId()))
			{
				auto attachmentImageView = get_resolved_image_view(
				    task_index, render_pass_desc.depth_attachment.image_view_proxy_id);
				if (image_view_contains_subresource(attachmentImageView, image_data, mip_level, array_layer))
				{
					return ImageUsageTypes::eDepthAttachment;
				}
			}

			for (const auto &image_view_proxy : render_pass_desc.input_image_view_proxies)
			{
				if (image_view_contains_subresource(get_resolved_image_view(task_index, image_view_proxy),
				                                    image_data,
				                                    mip_level, array_layer))
					return ImageUsageTypes::eGraphicsShaderRead;
			}

			for (const auto &image_view_proxy : render_pass_desc.inout_storage_image_proxies)
			{
				if (image_view_contains_subresource(get_resolved_image_view(task_index, image_view_proxy),
				                                    image_data,
				                                    mip_level, array_layer))
					return ImageUsageTypes::eGraphicsShaderReadWrite;
			}
		}
		break;
		case Task::Types::eComputePass:
		{
			const auto &compute_pass_desc = compute_pass_descs_[task.index];
			for (const auto &image_view_proxy : compute_pass_desc.input_image_view_proxies)
			{
				if (image_view_contains_subresource(get_resolved_image_view(task_index, image_view_proxy),
				                                    image_data,
				                                    mip_level, array_layer))
					return ImageUsageTypes::eComputeShaderRead;
			}
			for (const auto &image_view_proxy : compute_pass_desc.inout_storage_image_proxies)
			{
				if (image_view_contains_subresource(get_resolved_image_view(task_index, image_view_proxy),
				                                    image_data,
				                                    mip_level, array_layer))
					return ImageUsageTypes::eComputeShaderReadWrite;
			}
		}
		break;
		case Task::Types::eTransferPass:
		{
			const auto &transfer_pass_desc = transfer_pass_descs_[task.index];
			for (const auto &src_image_view_proxy : transfer_pass_desc.src_image_view_proxies)
			{
				if (image_view_contains_subresource(get_resolved_image_view(task_index, src_image_view_proxy),
				                                    image_data,
				                                    mip_level, array_layer))
					return ImageUsageTypes::eTransferSrc;
			}
			for (const auto &dst_image_view_proxy : transfer_pass_desc.dst_image_view_proxies)
			{
				if (image_view_contains_subresource(get_resolved_image_view(task_index, dst_image_view_proxy),
				                                    image_data,
				                                    mip_level, array_layer))
					return ImageUsageTypes::eTransferDst;
			}
		}
		break;
		case Task::Types::eImagePresent:
		{
			const auto &image_present_desc = image_present_descs_[task.index];
			if (image_view_contains_subresource(
			        get_resolved_image_view(task_index, image_present_desc.present_image_view_proxy_id), image_data,
			        mip_level,
			        array_layer))
				return ImageUsageTypes::ePresent;
		}
		break;
	}

	return ImageUsageTypes::eNone;
}

BufferUsageTypes RenderGraph::get_task_buffer_usage_type(size_t task_index, lz::Buffer *buffer)
{
	const Task &task = tasks_[task_index];

	switch (task.type)
	{
		case Task::Types::eRenderPass:
		{
			const auto &render_pass_desc = render_pass_descs_[task.index];
			for (const auto &storage_buffer_proxy : render_pass_desc.inout_storage_buffer_proxies)
			{
				const auto storage_buffer = get_resolved_buffer(task_index, storage_buffer_proxy);
				if (buffer->get_handle() == storage_buffer->get_handle())
				{
					return BufferUsageTypes::eGraphicsShaderReadWrite;
				}
			}

			for (const auto &vertex_buffer_proxy : render_pass_desc.vertex_buffer_proxies)
			{
				const auto vertex_buffer = get_resolved_buffer(task_index, vertex_buffer_proxy);
				if (buffer->get_handle() == vertex_buffer->get_handle())
				{
					return BufferUsageTypes::eVertexBuffer;
				}
			}
		}
		break;
		case Task::Types::eComputePass:
		{
			const auto &compute_pass_desc = compute_pass_descs_[task.index];
			for (const auto storage_buffer_proxy : compute_pass_desc.inout_storage_buffer_proxies)
			{
				const auto storage_buffer = get_resolved_buffer(task_index, storage_buffer_proxy);
				if (buffer->get_handle() == storage_buffer->get_handle())
					return BufferUsageTypes::eComputeShaderReadWrite;
			}
		}
		break;

		case Task::Types::eTransferPass:
		{
			const auto &transfer_pass_desc = transfer_pass_descs_[task.index];
			for (const auto src_buffer_proxy : transfer_pass_desc.src_buffer_proxies)
			{
				const auto src_buffer = get_resolved_buffer(task_index, src_buffer_proxy);
				if (buffer->get_handle() == src_buffer->get_handle())
					return BufferUsageTypes::eTransferSrc;
			}
			for (const auto dst_buffer_proxy : transfer_pass_desc.dst_buffer_proxies)
			{
				const auto dst_buffer = get_resolved_buffer(task_index, dst_buffer_proxy);
				if (buffer->get_handle() == dst_buffer->get_handle())
					return BufferUsageTypes::eTransferSrc;
			}
		}
		break;
	}
	return BufferUsageTypes::eNone;
}

ImageUsageTypes RenderGraph::get_last_image_subresource_usage_type(size_t task_index, lz::ImageData *image_data,
                                                                   uint32_t mip_level, uint32_t array_layer)
{
	for (size_t task_offset = 0; task_offset < task_index; task_offset++)
	{
		const size_t prev_task_index = task_index - task_offset - 1;
		const auto   usage_type      = get_task_image_subresource_usage_type(
            prev_task_index, image_data, mip_level, array_layer);
		if (usage_type != ImageUsageTypes::eNone)
			return usage_type;
	}

	for (const auto &image_view_proxy : image_view_proxies_)
	{
		if (image_view_proxy.type == ImageViewProxy::Types::eExternal && image_view_proxy.external_view->get_image_data() == image_data)
		{
			return image_view_proxy.external_usage_type;
		}
	}
	return ImageUsageTypes::eNone;
}

BufferUsageTypes RenderGraph::get_last_buffer_usage_type(size_t task_index, lz::Buffer *buffer)
{
	for (size_t task_offset = 1; task_offset < task_index; task_offset++)
	{
		const size_t prev_task_index = task_index - task_offset;
		const auto   usage_type      = get_task_buffer_usage_type(prev_task_index, buffer);
		if (usage_type != BufferUsageTypes::eNone)
			return usage_type;
	}
	return BufferUsageTypes::eNone;
}

void RenderGraph::flush_image_transition_barriers(lz::ImageData *image_data, vk::ImageSubresourceRange range,
                                                  ImageUsageTypes src_usage_type, ImageUsageTypes dst_usage_type,
                                                  vk::PipelineStageFlags              &src_stage,
                                                  vk::PipelineStageFlags              &dst_stage,
                                                  std::vector<vk::ImageMemoryBarrier> &image_barriers)
{
	if (is_image_barrier_needed(src_usage_type, dst_usage_type) && range.layerCount > 0 && range.levelCount > 0)
	{
		const auto src_image_access_pattern = get_src_image_access_pattern(src_usage_type);
		const auto dst_image_access_pattern = get_dst_image_access_pattern(dst_usage_type);
		auto       image_barrier            = vk::ImageMemoryBarrier()
		                         .setSrcAccessMask(src_image_access_pattern.access_mask)
		                         .setDstAccessMask(dst_image_access_pattern.access_mask)
		                         .setOldLayout(src_image_access_pattern.layout)
		                         .setNewLayout(dst_image_access_pattern.layout)
		                         .setSubresourceRange(range)
		                         .setImage(image_data->get_handle());

		if (src_image_access_pattern.queue_family_type == dst_image_access_pattern.queue_family_type)
		{
			image_barrier
			    .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			    .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
		}
		else
		{
			// TODO: transfer queue
			image_barrier
			    .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			    .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
		}

		src_stage |= src_image_access_pattern.stage;
		dst_stage |= dst_image_access_pattern.stage;

		image_barriers.push_back(image_barrier);
	}
}

void RenderGraph::add_image_transition_barriers(lz::ImageView *image_view, ImageUsageTypes dst_usage_type,
                                                size_t dst_task_index, vk::PipelineStageFlags &src_stage,
                                                vk::PipelineStageFlags              &dst_stage,
                                                std::vector<vk::ImageMemoryBarrier> &image_barriers)
{
	auto range = vk::ImageSubresourceRange()
	                 .setAspectMask(image_view->get_image_data()->get_aspect_flags());

	for (uint32_t array_layer = image_view->get_base_array_layer(); array_layer < image_view->get_base_array_layer() +
	                                                                                  image_view->get_array_layers_count();
	     ++array_layer)
	{
		range.setBaseArrayLayer(array_layer)
		    .setLayerCount(1)
		    .setBaseMipLevel(image_view->get_base_mip_level())
		    .setLevelCount(0);
		ImageUsageTypes prev_subresource_usage_type = ImageUsageTypes::eNone;

		for (uint32_t mip_level = image_view->get_base_mip_level(); mip_level < image_view->get_base_mip_level() +
		                                                                            image_view->get_mip_levels_count();
		     ++mip_level)
		{
			const auto last_usage_type = get_last_image_subresource_usage_type(
			    dst_task_index, image_view->get_image_data(), mip_level, array_layer);
			if (prev_subresource_usage_type != last_usage_type)
			{
				flush_image_transition_barriers(image_view->get_image_data(),
				                                range, prev_subresource_usage_type, dst_usage_type, src_stage,
				                                dst_stage,
				                                image_barriers);
				range.setBaseMipLevel(mip_level)
				    .setLevelCount(0);
				prev_subresource_usage_type = last_usage_type;
			}
			range.levelCount++;
		}
		flush_image_transition_barriers(image_view->get_image_data(), range, prev_subresource_usage_type,
		                                dst_usage_type,
		                                src_stage, dst_stage, image_barriers);
	}
}

void RenderGraph::flush_buffer_transition_barriers(lz::Buffer *buffer, BufferUsageTypes src_usage_type,
                                                   BufferUsageTypes                      dst_usage_type,
                                                   vk::PipelineStageFlags               &src_stage,
                                                   vk::PipelineStageFlags               &dst_stage,
                                                   std::vector<vk::BufferMemoryBarrier> &buffer_barriers)
{
	if (is_buffer_barrier_needed(src_usage_type, dst_usage_type))
	{
		const auto src_buffer_access_pattern = get_src_buffer_access_pattern(src_usage_type);
		const auto dst_buffer_access_pattern = get_dst_buffer_access_pattern(dst_usage_type);
		auto       buffer_barrier            = vk::BufferMemoryBarrier()
		                          .setSrcAccessMask(src_buffer_access_pattern.access_mask)
		                          .setOffset(0)
		                          .setSize(VK_WHOLE_SIZE)
		                          .setDstAccessMask(dst_buffer_access_pattern.access_mask)
		                          .setBuffer(buffer->get_handle());

		if (src_buffer_access_pattern.queue_family_type == dst_buffer_access_pattern.queue_family_type)
		{
			buffer_barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			    .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
		}
		else
		{
			// TODO: transfer queue
			buffer_barrier
			    .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			    .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
		}

		src_stage |= src_buffer_access_pattern.stage;
		dst_stage |= dst_buffer_access_pattern.stage;
		buffer_barriers.push_back(buffer_barrier);
	}
}

void RenderGraph::add_buffer_barriers(lz::Buffer *buffer, BufferUsageTypes dstUsageType, size_t dst_task_index,
                                      vk::PipelineStageFlags &src_stage, vk::PipelineStageFlags &dst_stage,
                                      std::vector<vk::BufferMemoryBarrier> &buffer_barriers)
{
	const auto last_usage_type = get_last_buffer_usage_type(dst_task_index, buffer);
	flush_buffer_transition_barriers(buffer, last_usage_type, dstUsageType, src_stage, dst_stage, buffer_barriers);
}

bool RenderGraph::ImageViewProxy::contains(const ImageViewProxy &other)
{
	if (type == Types::eTransient && subresource_range.contains(other.subresource_range) && type == other.type && image_proxy_id == other.image_proxy_id)
	{
		return true;
	}

	if (type == Types::eExternal && external_view == other.external_view)
	{
		return true;
	}
	return false;
}

void RenderGraph::resolve_images()
{
	image_cache_.release();

	for (auto &image_proxy : image_proxies_)
	{
		switch (image_proxy.type)
		{
			case ImageProxy::Types::eExternal:
			{
				image_proxy.resolved_image = image_proxy.external_image;
			}
			break;
			case ImageProxy::Types::eTransient:
			{
				image_proxy.resolved_image = image_cache_.get_image(image_proxy.image_key);
			}
			break;
		}
	}
}

lz::ImageData *RenderGraph::get_resolved_image(size_t task_index, ImageProxyId image_proxy)
{
	return image_proxies_.get(image_proxy).resolved_image;
}

void RenderGraph::resolve_image_views()
{
	for (auto &image_view_proxy : image_view_proxies_)
	{
		switch (image_view_proxy.type)
		{
			case ImageViewProxy::Types::eExternal:
			{
				image_view_proxy.resolved_image_view = image_view_proxy.external_view;
			}
			break;
			case ImageViewProxy::Types::eTransient:
			{
				ImageViewCache::ImageViewKey imageViewKey;
				imageViewKey.image             = get_resolved_image(0, image_view_proxy.image_proxy_id);
				imageViewKey.subresource_range = image_view_proxy.subresource_range;
				imageViewKey.debug_name        = image_view_proxy.debug_name + "[img: " + image_proxies_.get(image_view_proxy.image_proxy_id).image_key.debug_name + "]";

				image_view_proxy.resolved_image_view = image_view_cache_.get_image_view(imageViewKey);
			}
			break;
		}
	}
}

lz::ImageView *RenderGraph::get_resolved_image_view(size_t task_index, ImageViewProxyId image_view_proxy_id)
{
	return image_view_proxies_.get(image_view_proxy_id).resolved_image_view;
}

void RenderGraph::resolve_buffers()
{
	buffer_cache_.release();

	for (auto &buffer_proxy : buffer_proxies_)
	{
		switch (buffer_proxy.type)
		{
			case BufferProxy::Types::eExternal:
			{
				buffer_proxy.resolved_buffer = buffer_proxy.external_buffer;
			}
			break;
			case BufferProxy::Types::eTransient:
			{
				buffer_proxy.resolved_buffer = buffer_cache_.get_buffer(buffer_proxy.buffer_key);
			}
			break;
		}
	}
}

lz::Buffer *RenderGraph::get_resolved_buffer(size_t task_index, BufferProxyId buffer_proxy_id)
{
	return buffer_proxies_.get(buffer_proxy_id).resolved_buffer;
}

void RenderGraph::add_task(Task task)
{
	tasks_.push_back(task);
}

lz::ProfilerTask RenderGraph::create_profiler_task(const RenderPassDesc &render_pass_desc)
{
	lz::ProfilerTask task;
	task.start_time = -1.0f;
	task.end_time   = -1.0f;
	task.name       = render_pass_desc.profiler_task_name;
	task.color      = render_pass_desc.profiler_task_color;
	return task;
}

lz::ProfilerTask RenderGraph::create_profiler_task(const ComputePassDesc &compute_pass_desc)
{
	lz::ProfilerTask task;
	task.start_time = -1.0f;
	task.end_time   = -1.0f;
	task.name       = compute_pass_desc.profiler_task_name;
	task.color      = compute_pass_desc.profiler_task_color;
	return task;
}

lz::ProfilerTask RenderGraph::create_profiler_task(const TransferPassDesc &transfer_pass_desc)
{
	lz::ProfilerTask task;
	task.start_time = -1.0f;
	task.end_time   = -1.0f;
	task.name       = transfer_pass_desc.profiler_task_name;
	task.color      = transfer_pass_desc.profiler_task_color;
	return task;
}

lz::ProfilerTask RenderGraph::create_profiler_task(const ImagePresentPassDesc &image_present_pass_desc)
{
	lz::ProfilerTask task;
	task.start_time = -1.0f;
	task.end_time   = -1.0f;
	task.name       = "ImagePresent";
	task.color      = glm::packUnorm4x8(glm::vec4(0.0f, 1.0f, 0.5f, 1.0f));
	return task;
}

lz::ProfilerTask RenderGraph::create_profiler_task(const FrameSyncBeginPassDesc &frame_sync_begin_pass_desc)
{
	lz::ProfilerTask task;
	task.start_time = -1.0f;
	task.end_time   = -1.0f;
	task.name       = "FrameSyncBegin";
	task.color      = glm::packUnorm4x8(glm::vec4(0.0f, 0.5f, 1.0f, 1.0f));
	return task;
}

lz::ProfilerTask RenderGraph::create_profiler_task(const FrameSyncEndPassDesc &frame_sync_end_pass_desc)
{
	lz::ProfilerTask task;
	task.start_time = -1.0f;
	task.end_time   = -1.0f;
	task.name       = "FrameSyncEnd";
	task.color      = glm::packUnorm4x8(glm::vec4(0.0f, 0.5f, 1.0f, 1.0f));
	return task;
}
}        // namespace lz
