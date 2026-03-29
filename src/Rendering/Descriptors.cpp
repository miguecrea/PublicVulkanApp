#include "Descriptors.h"
#include "../Core/Device.h"
#include "UniformBuffer.h"
#include <stdexcept>

// -------------------------------------------------------
// Camera layout — binding 0: UBO (vertex)
// -------------------------------------------------------
void Descriptors::CreateCameraLayout(Device* device)
{
    m_Device = device;

    VkDescriptorSetLayoutBinding ubo{};
    ubo.binding = 0;
    ubo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo.descriptorCount = 1;
    ubo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 1;
    info.pBindings = &ubo;

    if (vkCreateDescriptorSetLayout(device->GetLogical(), &info, nullptr, &m_CameraLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create camera descriptor layout!");
}

void Descriptors::CreateCameraPool(Device* device, int framesInFlight)
{
    VkDescriptorPoolSize size{};
    size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    size.descriptorCount = framesInFlight;

    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.poolSizeCount = 1;
    info.pPoolSizes = &size;
    info.maxSets = framesInFlight;

    if (vkCreateDescriptorPool(device->GetLogical(), &info, nullptr, &m_CameraPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create camera descriptor pool!");
}

void Descriptors::CreateCameraSets(Device* device, int framesInFlight,
    const std::vector<VkBuffer>& cameraBuffers)
{
    std::vector<VkDescriptorSetLayout> layouts(framesInFlight, m_CameraLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_CameraPool;
    allocInfo.descriptorSetCount = framesInFlight;
    allocInfo.pSetLayouts = layouts.data();

    m_CameraSets.resize(framesInFlight);
    if (vkAllocateDescriptorSets(device->GetLogical(), &allocInfo, m_CameraSets.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate camera descriptor sets!");

    for (int i = 0; i < framesInFlight; i++)
    {
        VkDescriptorBufferInfo bufInfo{};
        bufInfo.buffer = cameraBuffers[i];
        bufInfo.offset = 0;
        bufInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_CameraSets[i];
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &bufInfo;

        vkUpdateDescriptorSets(device->GetLogical(), 1, &write, 0, nullptr);
    }
}

void Descriptors::DestroyCameraLayout()
{
    vkDestroyDescriptorSetLayout(m_Device->GetLogical(), m_CameraLayout, nullptr);
}

void Descriptors::DestroyCameraPool()
{
    vkDestroyDescriptorPool(m_Device->GetLogical(), m_CameraPool, nullptr);
}

// -------------------------------------------------------
// Material layout
// binding 0: albedo sampler (fragment)
// binding 1: normal sampler (fragment)
// binding 2: material UBO  (fragment)
// -------------------------------------------------------
void Descriptors::CreateMaterialLayout(Device* device)
{
    m_Device = device;

    std::array<VkDescriptorSetLayoutBinding, 3> bindings{};

    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 3;
    info.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device->GetLogical(), &info, nullptr, &m_MaterialLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create material descriptor layout!");
}

void Descriptors::CreateMaterialPool(Device* device, int materialCount)
{
    std::array<VkDescriptorPoolSize, 2> sizes{};
    sizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sizes[0].descriptorCount = materialCount * 2; // albedo + normal
    sizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    sizes[1].descriptorCount = materialCount;

    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.poolSizeCount = 2;
    info.pPoolSizes = sizes.data();
    info.maxSets = materialCount;

    if (vkCreateDescriptorPool(device->GetLogical(), &info, nullptr, &m_MaterialPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create material descriptor pool!");
}

void Descriptors::CreateMaterialSets(Device* device, int materialCount,
    const std::vector<VkBuffer>& materialUBOs,
    const std::vector<VkImageView>& albedoViews,
    const std::vector<VkSampler>& albedoSamplers,
    const std::vector<VkImageView>& normalViews,
    const std::vector<VkSampler>& normalSamplers,
    VkImageView fallbackView, VkSampler fallbackSampler)
{
    std::vector<VkDescriptorSetLayout> layouts(materialCount, m_MaterialLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_MaterialPool;
    allocInfo.descriptorSetCount = materialCount;
    allocInfo.pSetLayouts = layouts.data();

    m_MaterialSets.resize(materialCount);
    if (vkAllocateDescriptorSets(device->GetLogical(), &allocInfo, m_MaterialSets.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate material descriptor sets!");

    for (int i = 0; i < materialCount; i++)
    {
        // Albedo
        VkDescriptorImageInfo albedoInfo{};
        albedoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        albedoInfo.imageView = albedoViews[i] != VK_NULL_HANDLE ? albedoViews[i] : fallbackView;
        albedoInfo.sampler = albedoSamplers[i] != VK_NULL_HANDLE ? albedoSamplers[i] : fallbackSampler;

        // Normal
        VkDescriptorImageInfo normalInfo{};
        normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        normalInfo.imageView = normalViews[i] != VK_NULL_HANDLE ? normalViews[i] : fallbackView;
        normalInfo.sampler = normalSamplers[i] != VK_NULL_HANDLE ? normalSamplers[i] : fallbackSampler;

        // Material UBO
        VkDescriptorBufferInfo matInfo{};
        matInfo.buffer = materialUBOs[i];
        matInfo.offset = 0;
        matInfo.range = sizeof(MaterialUBO);

        std::array<VkWriteDescriptorSet, 3> writes{};

        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = m_MaterialSets[i];
        writes[0].dstBinding = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[0].descriptorCount = 1;
        writes[0].pImageInfo = &albedoInfo;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = m_MaterialSets[i];
        writes[1].dstBinding = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].descriptorCount = 1;
        writes[1].pImageInfo = &normalInfo;

        writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[2].dstSet = m_MaterialSets[i];
        writes[2].dstBinding = 2;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[2].descriptorCount = 1;
        writes[2].pBufferInfo = &matInfo;

        vkUpdateDescriptorSets(device->GetLogical(), 3, writes.data(), 0, nullptr);
    }
}

void Descriptors::DestroyMaterialLayout()
{
    vkDestroyDescriptorSetLayout(m_Device->GetLogical(), m_MaterialLayout, nullptr);
}

void Descriptors::DestroyMaterialPool()
{
    vkDestroyDescriptorPool(m_Device->GetLogical(), m_MaterialPool, nullptr);
}

// -------------------------------------------------------
// Lighting layout
// binding 0-3: input attachments
// binding 4: light UBO (per frame)
// -------------------------------------------------------
void Descriptors::CreateLightingLayout(Device* device)
{
    m_Device = device;

    std::array<VkDescriptorSetLayoutBinding, 5> bindings{};
    for (int i = 0; i < 4; i++)
    {
        bindings[i].binding = i;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    bindings[4].binding = 4;
    bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[4].descriptorCount = 1;
    bindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 5;
    info.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device->GetLogical(), &info, nullptr, &m_LightingLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create lighting descriptor layout!");
}

void Descriptors::CreateLightingPool(Device* device)
{
    std::array<VkDescriptorPoolSize, 2> sizes{};
    sizes[0].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    sizes[0].descriptorCount = 4 * 2; // 4 attachments * MAX_FRAMES
    sizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    sizes[1].descriptorCount = 2; // MAX_FRAMES

    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.poolSizeCount = 2;
    info.pPoolSizes = sizes.data();
    info.maxSets = 2; // one per frame

    if (vkCreateDescriptorPool(device->GetLogical(), &info, nullptr, &m_LightingPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create lighting descriptor pool!");
}

void Descriptors::CreateLightingSet(Device* device,
    const std::array<VkImageView, 4>& gbufferViews,
    const std::vector<VkBuffer>& lightBuffers, int framesInFlight)
{
    std::vector<VkDescriptorSetLayout> layouts(framesInFlight, m_LightingLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_LightingPool;
    allocInfo.descriptorSetCount = framesInFlight;
    allocInfo.pSetLayouts = layouts.data();

    m_LightingSets.resize(framesInFlight);
    if (vkAllocateDescriptorSets(device->GetLogical(), &allocInfo, m_LightingSets.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate lighting descriptor sets!");

    for (int f = 0; f < framesInFlight; f++)
    {
        std::array<VkDescriptorImageInfo, 4> imageInfos{};
        std::array<VkWriteDescriptorSet, 5> writes{};

        for (int i = 0; i < 4; i++)
        {
            imageInfos[i].imageView = gbufferViews[i];
            imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[i].dstSet = m_LightingSets[f];
            writes[i].dstBinding = i;
            writes[i].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            writes[i].descriptorCount = 1;
            writes[i].pImageInfo = &imageInfos[i];
        }

        VkDescriptorBufferInfo lightInfo{};
        lightInfo.buffer = lightBuffers[f];
        lightInfo.offset = 0;
        lightInfo.range = sizeof(LightUBO);

        writes[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[4].dstSet = m_LightingSets[f];
        writes[4].dstBinding = 4;
        writes[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[4].descriptorCount = 1;
        writes[4].pBufferInfo = &lightInfo;

        vkUpdateDescriptorSets(device->GetLogical(), 5, writes.data(), 0, nullptr);
    }
}

void Descriptors::DestroyLightingLayout()
{
    vkDestroyDescriptorSetLayout(m_Device->GetLogical(), m_LightingLayout, nullptr);
}

void Descriptors::DestroyLightingPool()
{
    vkDestroyDescriptorPool(m_Device->GetLogical(), m_LightingPool, nullptr);
    m_LightingSets.clear();
}