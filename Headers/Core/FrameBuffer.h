#pragma once
#include <vector>
#include <vulkan/vulkan.h>

class FramebufferManager
{
public:
    FramebufferManager(VkDevice device);
    ~FramebufferManager() = default;

    void CreateFramebuffers(
        VkRenderPass renderPass,
        const std::vector<VkImageView>& swapChainImageViews,
        VkExtent2D extent
    );

    const std::vector<VkFramebuffer>& GetFrameBuffers();

    void DestroyFrameBuffers();

private:
    VkDevice m_Device;
    std::vector<VkFramebuffer> swapChainFramebuffers;
};