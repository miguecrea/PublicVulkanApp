#include "GBuffer.h"
#include "../Core/Device.h"
#include "../Resources/Texture.h"
#include <stdexcept>

void GBuffer::Create(Device* device, VkExtent2D extent)
{
    m_Device = device;
    for (int i = 0; i < Count; i++)
        CreateAttachment(i, extent);
}

void GBuffer::Destroy()
{
    for (int i = 0; i < Count; i++)
    {
        vkDestroyImageView(m_Device->GetLogical(), m_Views[i], nullptr);
        vkDestroyImage(m_Device->GetLogical(), m_Images[i], nullptr);
        vkFreeMemory(m_Device->GetLogical(), m_Memories[i], nullptr);
    }
}

void GBuffer::CreateAttachment(int index, VkExtent2D extent)
{
    VkFormat format = Formats[index];

    Texture::CreateImage(
        m_Device,
        extent.width, extent.height,
        format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_Images[index],
        m_Memories[index]
    );

    m_Views[index] = Texture::CreateImageView(
        m_Device,
        m_Images[index],
        format,
        VK_IMAGE_ASPECT_COLOR_BIT
    );
}
