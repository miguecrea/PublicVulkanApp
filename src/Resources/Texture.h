#pragma once
#include <vulkan/vulkan.h>
#include <string>

class Device;
class CommandManager;

class Texture
{
public:
    Texture() = default;
    ~Texture() = default;

    void Load(Device* device, CommandManager* cmdManager, const std::string& path);
    void Destroy();

    VkImageView GetView()    const { return m_View; }
    VkSampler   GetSampler() const { return m_Sampler; }

    // Reusable image utilities
    static void CreateImage(Device* device, uint32_t width, uint32_t height,
        VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& memory);

    static VkImageView CreateImageView(Device* device, VkImage image,
        VkFormat format, VkImageAspectFlags aspectFlags);

    static void TransitionImageLayout(Device* device, CommandManager* cmdManager,
        VkImage image, VkFormat format,
        VkImageLayout oldLayout, VkImageLayout newLayout);

private:
    Device* m_Device = nullptr;

    VkImage        m_Image  = VK_NULL_HANDLE;
    VkDeviceMemory m_Memory = VK_NULL_HANDLE;
    VkImageView    m_View   = VK_NULL_HANDLE;
    VkSampler      m_Sampler = VK_NULL_HANDLE;

    void CopyBufferToImage(CommandManager* cmdManager, VkBuffer buffer,
        VkImage image, uint32_t width, uint32_t height);
    void CreateSampler();
};
