#pragma once
//
//class BaseRenderer
//{
//public:
//	BaseRenderer(){}
//
//	virtual ~BaseRenderer() {}
//	virtual void RecreateSceneResources(Scene* scene) {}
//	virtual void RecreateSwapchainResources(vk::Extent2D viewportExtent, size_t inFlightFramesCount) {}
//	virtual void RenderFrame(const lz::InFlightQueue::FrameInfo& frameInfo, const Camera& camera, const Camera& light, Scene* scene, GLFWwindow* window) {}
//	virtual void ReloadShaders() {}
//	virtual void ChangeView() {}
//};