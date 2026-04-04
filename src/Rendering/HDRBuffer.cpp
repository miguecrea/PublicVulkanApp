#include "HDRBuffer.h"
#include "../Core/Device.h"
#include "../Resources/Texture.h"

void HDRBuffer::Create(Device* device, VkExtent2D extent)
{
    m_Device = device;

    Texture::CreateImage(device,
        extent.width, extent.height,
        Format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_Image, m_Memory);

    m_View = Texture::CreateImageView(device, m_Image, Format, VK_IMAGE_ASPECT_COLOR_BIT);
}

void HDRBuffer::Destroy()
{
    vkDestroyImageView(m_Device->GetLogical(), m_View, nullptr);
    vkDestroyImage(m_Device->GetLogical(), m_Image, nullptr);
    vkFreeMemory(m_Device->GetLogical(), m_Memory, nullptr);
}