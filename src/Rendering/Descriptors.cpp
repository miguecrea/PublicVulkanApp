#include "Descriptors.h"
#include "../Core/Device.h"
#include "UniformBuffer.h"
#include <stdexcept>

void Descriptors::CreateLayout(Device* device)
{
    m_Device = device;

    VkDescriptorSetLayoutBinding uboBinding{};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding = 1;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.descriptorCount = 1;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboBinding, samplerBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device->GetLogical(), &layoutInfo, nullptr, &m_Layout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor set layout!");
}

void Descriptors::CreatePool(Device* device, int framesInFlight)
{
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(framesInFlight);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(framesInFlight);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(framesInFlight);

    if (vkCreateDescriptorPool(device->GetLogical(), &poolInfo, nullptr, &m_Pool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor pool!");
}

void Descriptors::CreateSets(Device* device, int framesInFlight,
    const std::vector<VkBuffer>& uniformBuffers,
    VkImageView textureView, VkSampler sampler)
{
    std::vector<VkDescriptorSetLayout> layouts(framesInFlight, m_Layout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_Pool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(framesInFlight);
    allocInfo.pSetLayouts = layouts.data();

    m_Sets.resize(framesInFlight);
    if (vkAllocateDescriptorSets(device->GetLogical(), &allocInfo, m_Sets.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate descriptor sets!");

    for (int i = 0; i < framesInFlight; i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureView;
        imageInfo.sampler = sampler;

        std::array<VkWriteDescriptorSet, 2> writes{};

        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = m_Sets[i];
        writes[0].dstBinding = 0;
        writes[0].dstArrayElement = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].descriptorCount = 1;
        writes[0].pBufferInfo = &bufferInfo;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = m_Sets[i];
        writes[1].dstBinding = 1;
        writes[1].dstArrayElement = 0;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].descriptorCount = 1;
        writes[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device->GetLogical(),
            static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}

void Descriptors::DestroyLayout()
{
    vkDestroyDescriptorSetLayout(m_Device->GetLogical(), m_Layout, nullptr);
}

void Descriptors::DestroyPool()
{
    vkDestroyDescriptorPool(m_Device->GetLogical(), m_Pool, nullptr);
}

// -------------------------------------------------------
// Lighting pass — 4 input attachments
// -------------------------------------------------------
void Descriptors::CreateLightingLayout(Device* device)
{
    m_Device = device;

    std::array<VkDescriptorSetLayoutBinding, 4> bindings{};
    for (int i = 0; i < 4; i++)
    {
        bindings[i].binding = i;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 4;
    info.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device->GetLogical(), &info, nullptr, &m_LightingLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create lighting descriptor layout!");
}

void Descriptors::CreateLightingPool(Device* device)
{
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    poolSize.descriptorCount = 4;

    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.poolSizeCount = 1;
    info.pPoolSizes = &poolSize;
    info.maxSets = 1;

    if (vkCreateDescriptorPool(device->GetLogical(), &info, nullptr, &m_LightingPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create lighting descriptor pool!");
}

void Descriptors::CreateLightingSet(Device* device, const std::array<VkImageView, 4>& gbufferViews)
{
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_LightingPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_LightingLayout;

    if (vkAllocateDescriptorSets(device->GetLogical(), &allocInfo, &m_LightingSet) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate lighting descriptor set!");

    std::array<VkDescriptorImageInfo, 4> imageInfos{};
    std::array<VkWriteDescriptorSet, 4> writes{};

    for (int i = 0; i < 4; i++)
    {
        imageInfos[i].imageView = gbufferViews[i];
        imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = m_LightingSet;
        writes[i].dstBinding = i;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        writes[i].descriptorCount = 1;
        writes[i].pImageInfo = &imageInfos[i];
    }

    vkUpdateDescriptorSets(device->GetLogical(), 4, writes.data(), 0, nullptr);
}

void Descriptors::DestroyLightingPool()
{
    vkDestroyDescriptorPool(m_Device->GetLogical(), m_LightingPool, nullptr);
    m_LightingSet = VK_NULL_HANDLE;
}

void Descriptors::DestroyLightingLayout()
{
    vkDestroyDescriptorSetLayout(m_Device->GetLogical(), m_LightingLayout, nullptr);
}