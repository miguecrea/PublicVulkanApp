#pragma once
#include <vulkan/vulkan.h>

class Device;
class CommandManager;

class ShadowMap
{
public:
    static constexpr uint32_t SIZE = 2048;

    ShadowMap()  = default;
    ~ShadowMap() = default;

    void Create(Device* device, CommandManager* cmdManager);
    void Destroy();

    VkRenderPass  GetRenderPass()  const { return m_RenderPass;  }
    VkFramebuffer GetFramebuffer() const { return m_Framebuffer; }
    VkImageView   GetView()        const { return m_View;        }
    VkSampler     GetSampler()     const { return m_Sampler;     }
    VkImage       GetImage()       const { return m_Image;       }

private:
    Device* m_Device = nullptr;

    VkImage        m_Image       = VK_NULL_HANDLE;
    VkDeviceMemory m_Memory      = VK_NULL_HANDLE;
    VkImageView    m_View        = VK_NULL_HANDLE;
    VkSampler      m_Sampler     = VK_NULL_HANDLE;
    VkRenderPass   m_RenderPass  = VK_NULL_HANDLE;
    VkFramebuffer  m_Framebuffer = VK_NULL_HANDLE;

    VkFormat FindDepthFormat() const;
    void TransitionLayout(CommandManager* cmdManager, VkFormat format,
                          VkImageLayout oldLayout, VkImageLayout newLayout);
};
