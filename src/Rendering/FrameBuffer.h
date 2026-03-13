#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class Device;

class FrameBuffer
{
public:
    FrameBuffer() = default;
    ~FrameBuffer() = default;

    void Create(Device* device, VkRenderPass renderPass,
        const std::vector<VkImageView>& colorViews,
        VkImageView depthView, VkExtent2D extent);
    void Destroy();

    const std::vector<VkFramebuffer>& Get() const { return m_Framebuffers; }

private:
    Device* m_Device = nullptr;
    std::vector<VkFramebuffer> m_Framebuffers;
};
