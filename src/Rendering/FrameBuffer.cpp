#include "FrameBuffer.h"
#include "../Core/Device.h"
#include <stdexcept>
#include <array>

void FrameBuffer::Create(Device* device, VkRenderPass renderPass,
    const std::vector<VkImageView>& colorViews,
    VkImageView depthView, VkExtent2D extent)
{
    m_Device = device;
    m_Framebuffers.resize(colorViews.size());

    for (size_t i = 0; i < colorViews.size(); i++)
    {
        std::array<VkImageView, 2> attachments = { colorViews[i], depthView };

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
