#pragma once
#include <vulkan/vulkan.h>

class Device;

class RenderPass
{
public:
    RenderPass() = default;
    ~RenderPass() = default;

    // colorFormat = swapchain format
    void Create(Device* device, VkFormat colorFormat);
    void Destroy();

    VkRenderPass Get() const { return m_RenderPass; }

private:
    Device* m_Device = nullptr;
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;

    VkFormat FindDepthFormat();
};