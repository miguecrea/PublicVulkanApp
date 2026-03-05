#pragma once 

#include <vulkan/vulkan.h>

class RenderPass 
{
public:

    RenderPass(VkDevice device, VkFormat swapChainImageFormat);
    ~RenderPass() = default;
    VkRenderPass Get() const { return m_RenderPass; }
    void CreateRenderPass();
    void DestroyRenderPass();
private:
    VkDevice m_Device;
    VkFormat m_ImageFormat;
    VkRenderPass m_RenderPass;

};