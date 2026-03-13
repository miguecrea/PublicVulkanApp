#include "Buffer.h"
#include "../Core/Device.h"
#include "../Core/CommandManager.h"
#include <stdexcept>
#include <cstring>

void Buffer::UploadMesh(Device* device, CommandManager* cmdManager, const Mesh& mesh)
{
    m_Device = device;
    m_IndexCount = static_cast<uint32_t>(mesh.indices.size());

    // ---- Vertex buffer ----
    VkDeviceSize vertSize = sizeof(mesh.vertices[0]) * mesh.vertices.size();
    VkBuffer stagingVert;
    VkDeviceMemory stagingVertMem;
    CreateBuffer(device, vertSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingVert, stagingVertMem);

    void* data;
    vkMapMemory(device->GetLogical(), stagingVertMem, 0, vertSize, 0, &data);
    memcpy(data, mesh.vertices.data(), vertSize);
    vkUnmapMemory(device->GetLogical(), stagingVertMem);

    CreateBuffer(device, vertSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_VertexBuffer, m_VertexMemory);

    CopyBuffer(cmdManager, stagingVert, m_VertexBuffer, vertSize);
    vkDestroyBuffer(device->GetLogical(), stagingVert, nullptr);
    vkFreeMemory(device->GetLogical(), stagingVertMem, nullptr);

    // ---- Index buffer ----
    VkDeviceSize idxSize = sizeof(mesh.indices[0]) * mesh.indices.size();
    VkBuffer stagingIdx;
    VkDeviceMemory stagingIdxMem;
    CreateBuffer(device, idxSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingIdx, stagingIdxMem);

    vkMapMemory(device->GetLogical(), stagingIdxMem, 0, idxSize, 0, &data);
    memcpy(data, mesh.indices.data(), idxSize);
    vkUnmapMemory(device->GetLogical(), stagingIdxMem);

    CreateBuffer(device, idxSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_IndexBuffer, m_IndexMemory);

    CopyBuffer(cmdManager, stagingIdx, m_IndexBuffer, idxSize);
    vkDestroyBuffer(device->GetLogical(), stagingIdx, nullptr);
    vkFreeMemory(device->GetLogical(), stagingIdxMem, nullptr);
}

void Buffer::Destroy()
{
    vkDestroyBuffer(m_Device->GetLogical(), m_IndexBuffer, nullptr);
    vkFreeMemory(m_Device->GetLogical(), m_IndexMemory, nullptr);
    vkDestroyBuffer(m_Device->GetLogical(), m_VertexBuffer, nullptr);
    vkFreeMemory(m_Device->GetLogical(), m_VertexMemory, nullptr);
}

void Buffer::CreateBuffer(Device* device, VkDeviceSize size,
    VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
    VkBuffer& buffer, VkDeviceMemory& memory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device->GetLogical(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to create buffer!");

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(device->GetLogical(), buffer, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = Device::FindMemoryType(device->GetPhysical(), memReqs.memoryTypeBits, properties);

    if (vkAllocateMemory(device->GetLogical(), &allocInfo, nullptr, &memory) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate buffer memory!");

    vkBindBufferMemory(device->GetLogical(), buffer, memory, 0);
}

void Buffer::CopyBuffer(CommandManager* cmdManager, VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
    VkCommandBuffer cmd = cmdManager->BeginSingleTimeCommands();
    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(cmd, src, dst, 1, &copyRegion);
    cmdManager->EndSingleTimeCommands(cmd);
}
