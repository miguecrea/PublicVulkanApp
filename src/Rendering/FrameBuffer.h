#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class Device;

class FrameBuffer
{
public:
    FrameBuffer() = default;
    ~FrameBuffer() = default;

    // colorViews: one per swapchain image
    // gbufferViews: [Position, Normal, Albedo, MetallicRoughness] - shared across frames
    // depthView: shared across frames
    void Create(Device* device, VkRenderPass renderPass,
        const std::vector<VkImageView>& swapchainViews,
        VkImageView depthView,
        const std::vector<VkImageView>& gbufferViews,
        VkExtent2D extent);
    void Destroy();

    const std::vector<VkFramebuffer>& Get() const { return m_Framebuffers; }

private:
    Device* m_Device = nullptr;
    std::vector<VkFramebuffer> m_Framebuffers;
};