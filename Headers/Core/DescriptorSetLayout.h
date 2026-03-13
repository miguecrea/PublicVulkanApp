
#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>
#include <string>
#include"glm/glm.hpp"


struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

class DescriptorSetLayout
{

public:
    DescriptorSetLayout(VkDevice device, VkPhysicalDevice physicalDevice, VkExtent2D swapChainExtend);
    void createDescriptorSetLayout();
    void DestroyDescriptorSetLayout();

    void  createUniformBuffers(int FramesInFlisght);
    void  DestroyUniformBuffers(int FramesInFlisght);

    void UpdateUniformBuffers(uint32_t currentFrame);
   void  createDescriptorPool(int FramesInFlight);
   void   DestroyDescriptorPool();
   void   createDescriptorSets(int FramesinFlight, VkImageView imageView, VkSampler sampler);


   std::vector<VkDescriptorSet> GetdescriptorSets();

    VkDescriptorSetLayout GetDescriptorSetLayout();


    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;
private:

    VkDevice              m_device;
    VkPhysicalDevice      m_PhysicalDevice;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout      pipelineLayout;

    VkExtent2D m_SwapChainExtend;
    VkDescriptorPool descriptorPool;

    std::vector<VkDescriptorSet> descriptorSets;

};

    // ----------------------------