#pragma once
#include <vulkan/vulkan.h>

class Device;

class HDRBuffer
{
public:
    static constexpr VkFormat Format = VK_FORMAT_R32G32B32A32_SFLOAT;

    void Create(Device* device, VkExtent2D extent);
    void Destroy();

    VkImageView GetView() const { return m_View; }

private:
    Device* m_Device = nullptr;
    VkImage        m_Image = VK_NULL_HANDLE;
    VkDeviceMemory m_Memory = VK_NULL_HANDLE;
    VkImageView    m_View = VK_NULL_HANDLE;
};