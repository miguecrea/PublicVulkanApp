#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class Device;

class CommandManager
{
public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    CommandManager() = default;
    ~CommandManager() = default;

    void Init(Device* device);
    void Destroy();

    void AllocateCommandBuffers();

    VkCommandPool GetPool() const { return m_Pool; }
    VkCommandBuffer GetBuffer(int frame) const { return m_Buffers[frame]; }

    // One-time command helpers
    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer cmd);

private:
    Device* m_Device = nullptr;
    VkCommandPool m_Pool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_Buffers;
};
