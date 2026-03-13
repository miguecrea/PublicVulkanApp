#include "DepthBuffer.h"
#include "Texture.h"
#include "../Core/Device.h"
#include <stdexcept>

void DepthBuffer::Create(Device* device, VkExtent2D extent)
{
    m_Device = device;
    VkFormat depthFormat = FindDepthFormat(device->GetPhysical());

    Texture::CreateImage(device, extent.width, extent.height, depthFormat,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Image, m_Memory);

    m_View = Texture::CreateImageView(device, m_Image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void DepthBuffer::Destroy()
{
    vkDestroyImageView(m_Device->GetLogical(), m_View, nullptr);
    vkDestroyImage(m_Device->GetLogical(), m_Image, nullptr);
    vkFreeMemory(m_Device->GetLogical(), m_Memory, nullptr);
}

VkFormat DepthBuffer::FindDepthFormat(VkPhysicalDevice physical)
{
    const std::vector<VkFormat> candidates = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physical, format, &props);
        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            return format;
    }
    throw std::runtime_error("Failed to find supported depth format!");
}
