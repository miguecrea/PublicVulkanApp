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

    void CreateLayout(Device* device);
    void CreatePool(Device* device, int framesInFlight);
    void CreateSets(Device* device, int framesInFlight,
        const std::vector<VkBuffer>& uniformBuffers,
        VkImageView textureView, VkSampler sampler);

    void DestroyLayout();
    void DestroyPool();

    VkDescriptorSetLayout GetLayout() const { return m_Layout; }
    VkDescriptorSet GetSet(int frame) const { return m_Sets[frame]; }

private:
    Device* m_Device = nullptr;
    VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
    VkDescriptorPool m_Pool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> m_Sets;
};
