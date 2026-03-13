#include "UniformBuffer.h"
#include "../Core/Device.h"
#include <stdexcept>
#include <cstring>

void UniformBuffer::Create(Device* device, int framesInFlight)
{
    m_Device = device;
    VkDeviceSize size = sizeof(UniformBufferObject);

    m_Buffers.resize(framesInFlight);
    m_Memory.resize(framesInFlight);
    m_Mapped.resize(framesInFlight);

    for (int i = 0; i < framesInFlight; i++)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device->GetLogical(), &bufferInfo, nullptr, &m_Buffers[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create uniform buffer!");

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(device->GetLogical(), m_Buffers[i], &memReqs);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = Device::FindMemoryType(device->GetPhysical(), memReqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(device->GetLogical(), &allocInfo, nullptr, &m_Memory[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate uniform buffer memory!");

        vkBindBufferMemory(device->GetLogical(), m_Buffers[i], m_Memory[i], 0);
        vkMapMemory(device->GetLogical(), m_Memory[i], 0, size, 0, &m_Mapped[i]);
    }
}

void UniformBuffer::Destroy(int framesInFlight)
{
    for (int i = 0; i < framesInFlight; i++)
    {
        vkDestroyBuffer(m_Device->GetLogical(), m_Buffers[i], nullptr);
        vkFreeMemory(m_Device->GetLogical(), m_Memory[i], nullptr);
    }
}

void UniformBuffer::Update(uint32_t frame, const UniformBufferObject& ubo)
{
    memcpy(m_Mapped[frame], &ubo, sizeof(ubo));
}
