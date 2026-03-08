#include"../Headers/Core/DescriptorSetLayout.h"
#include "../Headers/Core/Helpers.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>




DescriptorSetLayout::DescriptorSetLayout(VkDevice device,VkPhysicalDevice physicalDevice,VkExtent2D swapChainExtend):
    m_device{device},m_PhysicalDevice{physicalDevice},m_SwapChainExtend{swapChainExtend}
{
}

void DescriptorSetLayout::createDescriptorSetLayout()
{

    //UBO 
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    // binding index 
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;//is a uniform buffer
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  

    //IMAGE SAMPLER //we bind this to the SHADER IN THE FRAGMENT 
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; //in the fragmentr shader 

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };


    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}




void DescriptorSetLayout::createUniformBuffers(int FramesInFlisght)
{



    // for each frame in flight create a uniform buffer 

    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(FramesInFlisght);
    uniformBuffersMemory.resize(FramesInFlisght);
    uniformBuffersMapped.resize(FramesInFlisght);

    for (size_t i = 0; i < FramesInFlisght; i++)
    {
            createBuffer(m_device, m_PhysicalDevice, bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
            vkMapMemory(m_device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);

    }

}

void DescriptorSetLayout::DestroyUniformBuffers(int FramesInFlisght)
{

    //CREATE UBO wjat is on the shader 

    for (size_t i = 0; i < FramesInFlisght; i++) {
        vkDestroyBuffer(m_device, uniformBuffers[i], nullptr);
        vkFreeMemory(m_device, uniformBuffersMemory[i], nullptr);
    }
}

void DescriptorSetLayout::DestroyDescriptorSetLayout()
{
    vkDestroyDescriptorSetLayout(m_device, descriptorSetLayout, nullptr);
}

void DescriptorSetLayout::UpdateUniformBuffers(uint32_t currentFrame)
{

    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), m_SwapChainExtend.width / (float)m_SwapChainExtend.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;

    memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}



void DescriptorSetLayout::createDescriptorPool(int FramesInFlisght)
{



    // dec
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(FramesInFlisght);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = static_cast<uint32_t>(FramesInFlisght);

    if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }

}

void DescriptorSetLayout::DestroyDescriptorPool()
{
    vkDestroyDescriptorPool(m_device, descriptorPool, nullptr);

}

void DescriptorSetLayout::createDescriptorSets(int FramesinFlight, VkImageView imageView,VkSampler sampler)
{

    std::vector<VkDescriptorSetLayout> layouts(FramesinFlight, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(FramesinFlight);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(FramesinFlight);
    if (vkAllocateDescriptorSets(m_device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }


    //need to populate every descriptor 
    for (size_t i = 0; i < FramesinFlight; i++)
    
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

       
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = imageView;
        imageInfo.sampler = sampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

std::vector<VkDescriptorSet> DescriptorSetLayout::GetdescriptorSets()
{
    return descriptorSets;
}



VkDescriptorSetLayout DescriptorSetLayout::GetDescriptorSetLayout()
{
    return descriptorSetLayout;
}
