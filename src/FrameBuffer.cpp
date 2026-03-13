#include "../Headers/Core/FrameBuffer.h"
#include <stdexcept>
#include <iostream>
#include<array>



//An image view is sufficient to start using an image as a texture, but it's not quite ready to be used as a render target just yet. 
// That requires one more step of indirection, known as a framebuffer.
//But first we'll have to set up the graphics pipeline.

FramebufferManager::FramebufferManager(VkDevice device):
	m_Device{device}
{
}

void FramebufferManager::CreateFramebuffers(VkRenderPass renderPass, const std::vector<VkImageView>& swapChainImageViews, VkExtent2D extent,VkImageView depthImageView)
{
	swapChainFramebuffers.resize(swapChainImageViews.size());

    //We'll then iterate through the image views and create framebuffers from them:
    for (size_t i = 0; i < swapChainImageViews.size(); i++)
    {
       
        std::array<VkImageView, 2> attachments = 
        {
             swapChainImageViews[i],
           depthImageView
        };



        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}



const std::vector<VkFramebuffer> &  FramebufferManager::GetFrameBuffers()
{
    return swapChainFramebuffers;
}

void FramebufferManager::DestroyFrameBuffers()
{
    for (auto framebuffer : swapChainFramebuffers)
    {
        vkDestroyFramebuffer(m_Device, framebuffer, nullptr);
    }
}

