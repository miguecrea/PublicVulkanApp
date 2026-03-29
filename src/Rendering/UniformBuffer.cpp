#include "UniformBuffer.h"
#include "../Core/Device.h"
#include <stdexcept>
#include <cstring>

void UniformBuffer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags props, VkBuffer& buf, VkDeviceMemory& mem)
{
    VkBufferCreateInfo bufInfo{};
    bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufInfo.size = size;
    bufInfo.usage = usage;
    bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(m_Device->GetLogical(), &bufInfo, nullptr, &buf) != VK_SUCCESS)
        throw std::runtime_error("Failed to create uniform buffer!");

    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(m_Device->GetLogical(), buf, &req);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = req.size;
    allocInfo.memoryTypeIndex = FindMemoryType(req.memoryTypeBits, props);
    if (vkAllocateMemory(m_Device->GetLogical(), &allocInfo, nullptr, &mem) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate uniform buffer memory!");
    vkBindBufferMemory(m_Device->GetLogical(), buf, mem, 0);
}

uint32_t UniformBuffer::FindMemoryType(uint32_t filter, VkMemoryPropertyFlags props)
{
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(m_Device->GetPhysical(), &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
        if ((filter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & props) == props)
            return i;
    throw std::runtime_error("Failed to find suitable memory type!");
}

void UniformBuffer::Create(Device* device, int framesInFlight)
{
    m_Device = device;
    m_CameraBuffers.resize(framesInFlight);
    m_CameraMemories.resize(framesInFlight);
    m_CameraMapped.resize(framesInFlight);
    m_LightBuffers.resize(framesInFlight);
    m_LightMemories.resize(framesInFlight);
    m_LightMapped.resize(framesInFlight);

    for (int i = 0; i < framesInFlight; i++)
    {
        CreateBuffer(sizeof(UniformBufferObject),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_CameraBuffers[i], m_CameraMemories[i]);
        vkMapMemory(device->GetLogical(), m_CameraMemories[i], 0, sizeof(UniformBufferObject), 0, &m_CameraMapped[i]);

        CreateBuffer(sizeof(LightUBO),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_LightBuffers[i], m_LightMemories[i]);
        vkMapMemory(device->GetLogical(), m_LightMemories[i], 0, sizeof(LightUBO), 0, &m_LightMapped[i]);
    }
}

void UniformBuffer::CreateMaterialBuffers(Device* device, int count)
{
    m_Device = device;
    m_MaterialBuffers.resize(count);
    m_MaterialMemories.resize(count);
    m_MaterialMapped.resize(count);

    for (int i = 0; i < count; i++)
    {
        CreateBuffer(sizeof(MaterialUBO),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_MaterialBuffers[i], m_MaterialMemories[i]);
        vkMapMemory(device->GetLogical(), m_MaterialMemories[i], 0, sizeof(MaterialUBO), 0, &m_MaterialMapped[i]);
    }
}

void UniformBuffer::Update(uint32_t frame, const UniformBufferObject& ubo)
{
    memcpy(m_CameraMapped[frame], &ubo, sizeof(ubo));
}

void UniformBuffer::UpdateLight(uint32_t frame, const LightUBO& light)
{
    memcpy(m_LightMapped[frame], &light, sizeof(light));
}

void UniformBuffer::UpdateMaterial(int index, const MaterialUBO& mat)
{
    memcpy(m_MaterialMapped[index], &mat, sizeof(mat));
}

void UniformBuffer::DestroyMaterialBuffers()
{
    for (int i = 0; i < (int)m_MaterialBuffers.size(); i++)
    {
        vkUnmapMemory(m_Device->GetLogical(), m_MaterialMemories[i]);
        vkDestroyBuffer(m_Device->GetLogical(), m_MaterialBuffers[i], nullptr);
        vkFreeMemory(m_Device->GetLogical(), m_MaterialMemories[i], nullptr);
    }
    m_MaterialBuffers.clear();
    m_MaterialMemories.clear();
    m_MaterialMapped.clear();
}

void UniformBuffer::Destroy(int framesInFlight)
{
    for (int i = 0; i < framesInFlight; i++)
    {
        vkUnmapMemory(m_Device->GetLogical(), m_CameraMemories[i]);
        vkDestroyBuffer(m_Device->GetLogical(), m_CameraBuffers[i], nullptr);
        vkFreeMemory(m_Device->GetLogical(), m_CameraMemories[i], nullptr);

        vkUnmapMemory(m_Device->GetLogical(), m_LightMemories[i]);
        vkDestroyBuffer(m_Device->GetLogical(), m_LightBuffers[i], nullptr);
        vkFreeMemory(m_Device->GetLogical(), m_LightMemories[i], nullptr);
    }
    DestroyMaterialBuffers();
}