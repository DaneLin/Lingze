#pragma once
#include <map>

#include "Framebuffer.h"
#include "ImageView.h"
#include "RenderPass.h"

#include "Config.h"

namespace lz
{
class RenderPassCache
{
  public:
	struct RenderPassKey
	{
		RenderPassKey();

		std::vector<lz::RenderPass::AttachmentDesc> color_attachment_descs;
		lz::RenderPass::AttachmentDesc              depth_attachment_desc;

		bool operator<(const RenderPassKey &other) const;
	};

	RenderPassCache(vk::Device logical_device);

	lz::RenderPass *get_render_pass(const RenderPassKey &key);

  private:
	std::map<RenderPassKey, std::unique_ptr<lz::RenderPass>> render_pass_cache_;
	vk::Device                                               logical_device_;
};

class FramebufferCache
{
  public:
	struct PassInfo
	{
		lz::Framebuffer *framebuffer;
		lz::RenderPass  *render_pass;
	};

	struct Attachment
	{
		lz::ImageView *image_view;
		vk::ClearValue clear_value;
	};

	PassInfo begin_pass(vk::CommandBuffer command_buffer, const std::vector<Attachment> &color_attachments,
	                    Attachment *depth_attachment, lz::RenderPass *render_pass, vk::Extent2D render_area_extent);

	void end_pass(vk::CommandBuffer command_buffer);

	FramebufferCache(vk::Device logical_device);

  private:
	struct FramebufferKey
	{
		FramebufferKey();

		bool operator<(const FramebufferKey &other) const;

		std::array<const lz::ImageView *, 8> color_attachment_views;
		const lz::ImageView                 *depth_attachment_view;
		vk::Extent2D                         extent;
		vk::RenderPass                       render_pass;
	};

	lz::Framebuffer *get_framebuffer(FramebufferKey key);

	std::map<FramebufferKey, std::unique_ptr<lz::Framebuffer>> framebuffer_cache_;

	vk::Device logical_device_;
};
}        // namespace lz
