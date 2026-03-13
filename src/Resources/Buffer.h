#pragma once
#include <vulkan/vulkan.h>
#include "Mesh.h"

class Device;
class CommandManager;

class Buffer
{
public:
    Buffer() = default;
    ~Buffer() = default;

    void UploadMesh(Device* device, CommandManager* cmdManager, const Mesh& mesh);
    void Destroy();

    VkBuffer GetVertexBuffer() const { return m_VertexBuffer; }
    VkBuffer GetIndexBuffer()  const { return m_IndexBuffer; }
    uint32_t GetIndexCount()   const { return m_IndexCount; }

    // Static utility
    static void CreateBuffer(Device* device, VkDeviceSize size,
        VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
        VkBuffer& buffer, VkDeviceMemory& memory);

private:
    Device* m_Device = nullptr;

    VkBuffer       m_VertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_VertexMemory = VK_NULL_HANDLE;
    VkBuffer       m_IndexBuffer  = VK_NULL_HANDLE;
    VkDeviceMemory m_IndexMemory  = VK_NULL_HANDLE;
    uint32_t       m_IndexCount   = 0;

    void CopyBuffer(CommandManager* cmdManager, VkBuffer src, VkBuffer dst, VkDeviceSize size);
};
