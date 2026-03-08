#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>
#include "glm/glm.hpp"

struct UniformBufferObject 
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

class UniformBuffer 
{
public:
    UniformBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkExtent2D swapChainExtent);
    void createUniformBuffers(int framesInFlight);
    void DestroyUniformBuffers(int framesInFlight);
    void UpdateUniformBuffers(uint32_t currentFrame);

    std::vector<VkBuffer> & GetBuffers() { return m_uniformBuffers; }
    std::vector<VkDeviceMemory>& GetMemory() { return m_uniformBuffersMemory; }
    std::vector<void*>& GetMappedBuffers() { return m_uniformBuffersMapped; }

private:
    VkDevice         m_device;
    VkPhysicalDevice m_physicalDevice;
    VkExtent2D       m_swapChainExtent;

    std::vector<VkBuffer>       m_uniformBuffers;
    std::vector<VkDeviceMemory> m_uniformBuffersMemory;
    std::vector<void*>          m_uniformBuffersMapped;
};