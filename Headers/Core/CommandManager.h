#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include"QueueFamilyIndicesHeader.h"

class BufferManager;

class CommandManager
{
public:
    CommandManager(VkDevice device,const QueueFamilyIndices & queueFamilyIndices);
    ~CommandManager() = default;

    void CreateCommandPool();
    void DestroyCommandPool();
    void createCommandBuffer();

   const VkCommandBuffer & GetComandBuffer();

    //  that writes the commands we want to execute into a command buffer
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, VkRenderPass renderPass,
        const std::vector<VkFramebuffer>& frameBuffer,
        VkPipeline graphicspipeline, VkExtent2D extend,
        BufferManager * vertexbuffer, class DescriptorSetLayout * descriptorSet, VkPipelineLayout pipleinelayout);
    const std::vector<VkCommandBuffer> & GetCommandBuffersVector() const;


    VkCommandPool GetCommandPool();

    const int MAX_FRAMES_IN_FLIGHT = 2;
private:


    

    QueueFamilyIndices m_QueueFamilyIndices;
    VkDevice m_Device;

    VkCommandPool m_CommandPool;
    //command buffer are alocated from pools 
    VkCommandBuffer commandBuffer;

    std::vector<VkCommandBuffer> m_CommandBuffers;

    void CreateCommandPool(uint32_t queueFamilyIndex);
};