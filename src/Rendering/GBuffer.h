#pragma once
#include <vulkan/vulkan.h>
#include <array>

class Device;

class GBuffer
{
public:
    // G-Buffer attachment indices
    enum Attachment { Position = 0, Normal, Albedo, MetallicRoughness, Emissive, Count };

    // Formats — used by RenderPass and Pipeline too
    static constexpr VkFormat Formats[Count] =
    {
        VK_FORMAT_R32G32B32A32_SFLOAT, // Position
        VK_FORMAT_R16G16B16A16_SFLOAT, // Normal
        VK_FORMAT_R8G8B8A8_UNORM,      // Albedo
        VK_FORMAT_R8G8B8A8_UNORM,      // MetallicRoughness (R=metallic, G=roughness, B=AO)
        VK_FORMAT_R8G8B8A8_UNORM,      // Emissive
    };

    void Create(Device* device, VkExtent2D extent);
    void Destroy();

    VkImageView GetView(int index) const { return m_Views[index]; }

private:
    Device* m_Device = nullptr;

    std::array<VkImage,        Count> m_Images{};
    std::array<VkDeviceMemory, Count> m_Memories{};
    std::array<VkImageView,    Count> m_Views{};

    void CreateAttachment(int index, VkExtent2D extent);
};
