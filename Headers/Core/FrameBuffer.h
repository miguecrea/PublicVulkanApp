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

    void DestroyFrameBuffers();
   // const std::vector<VkFramebuffer>& GetFramebuffers() const;

private:
    VkDevice m_Device;
    std::vector<VkFramebuffer> swapChainFramebuffers;
};