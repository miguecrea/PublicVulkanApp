#pragma once
#include <vulkan/vulkan.h>

class Device;

class DepthBuffer
{
public:
    DepthBuffer() = default;
    ~DepthBuffer() = default;

    void Create(Device* device, VkExtent2D extent);
    void Destroy();

    VkImageView GetView() const { return m_View; }

    static VkFormat FindDepthFormat(VkPhysicalDevice physical);

private:
    Device* m_Device = nullptr;
    VkImage        m_Image  = VK_NULL_HANDLE;
    VkDeviceMemory m_Memory = VK_NULL_HANDLE;
    VkImageView    m_View   = VK_NULL_HANDLE;
};
