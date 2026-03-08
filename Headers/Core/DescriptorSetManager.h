

#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <stdexcept>

class DescriptorManager
{
public:
    DescriptorManager(VkDevice device);

    void createDescriptorPool(int framesInFlight);
    void createDescriptorSets(
        int framesInFlight,
        VkDescriptorSetLayout layout,
        std::vector<VkBuffer>& uniformBuffers,
        VkImageView imageView,
        VkSampler sampler
    );
    void DestroyDescriptorPool();

    std::vector<VkDescriptorSet>& GetDescriptorSets() { return m_descriptorSets; }
    VkPipelineLayout GetPipelineLayout() { return m_pipelineLayout; }

private:
    VkDevice             m_device;
    VkDescriptorPool     m_descriptorPool;
    VkPipelineLayout     m_pipelineLayout;
    std::vector<VkDescriptorSet> m_descriptorSets;
};