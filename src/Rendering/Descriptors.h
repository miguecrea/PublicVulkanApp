#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <array>

class Device;

class Descriptors
{
public:
    Descriptors() = default;
    ~Descriptors() = default;

    // --- Geometry pass (UBO + albedo texture) ---
    void CreateLayout(Device* device);
    void CreatePool(Device* device, int framesInFlight);
    void CreateSets(Device* device, int framesInFlight,
        const std::vector<VkBuffer>& uniformBuffers,
        VkImageView textureView, VkSampler sampler);
    void DestroyLayout();
    void DestroyPool();

    VkDescriptorSetLayout GetLayout()    const { return m_Layout; }
    VkDescriptorSet       GetSet(int frame) const { return m_Sets[frame]; }

    // --- Lighting pass (4 input attachments) ---
    void CreateLightingLayout(Device* device);
    void CreateLightingPool(Device* device);
    void CreateLightingSet(Device* device, const std::array<VkImageView, 4>& gbufferViews);
    void DestroyLightingPool();
    void DestroyLightingLayout();

    VkDescriptorSetLayout GetLightingLayout() const { return m_LightingLayout; }
    VkDescriptorSet       GetLightingSet()    const { return m_LightingSet; }

private:
    Device* m_Device = nullptr;

    // Geometry
    VkDescriptorSetLayout        m_Layout = VK_NULL_HANDLE;
    VkDescriptorPool             m_Pool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> m_Sets;

    // Lighting
    VkDescriptorSetLayout m_LightingLayout = VK_NULL_HANDLE;
    VkDescriptorPool      m_LightingPool = VK_NULL_HANDLE;
    VkDescriptorSet       m_LightingSet = VK_NULL_HANDLE;
};