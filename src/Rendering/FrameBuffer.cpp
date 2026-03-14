#include "FrameBuffer.h"
#include "../Core/Device.h"
#include <stdexcept>
#include <array>

void FrameBuffer::Create(Device* device, VkRenderPass renderPass,
    const std::vector<VkImageView>& swapchainViews,
    VkImageView depthView,
    const std::vector<VkImageView>& gbufferViews,
    VkExtent2D extent)
{
    m_Device = device;
    m_Framebuffers.resize(swapchainViews.size());

    for (size_t i = 0; i < swapchainViews.size(); i++)
    {
        // Attachment order must match render pass:
        // 0: swapchain color, 1: depth, 2-5: G-Buffer
        std::array<VkImageView, 6> attachments = {
            swapchainViews[i],   // 0 - swapchain
            depthView,           // 1 - depth
            gbufferViews[0],     // 2 - Position
            gbufferViews[1],     // 3 - Normal
            gbufferViews[2],     // 4 - Albedo
            gbufferViews[3],     // 5 - MetallicRoughness
        };

        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = renderPass;
        fbInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        fbInfo.pAttachments = attachments.data();
        fbInfo.width = extent.width;
        fbInfo.height = extent.height;
        fbInfo.layers = 1;

        if (vkCreateFramebuffer(device->GetLogical(), &fbInfo, nullptr, &m_Framebuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create framebuffer!");
    }
}

void FrameBuffer::Destroy()
{
    for (auto fb : m_Framebuffers)
        vkDestroyFramebuffer(m_Device->GetLogical(), fb, nullptr);
    m_Framebuffers.clear();
}