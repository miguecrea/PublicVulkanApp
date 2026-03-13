#include "CommandManager.h"
#include "Device.h"
#include <stdexcept>

void CommandManager::Init(Device * device)
{
    m_Device = device;

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = device->GetQueueFamilies().graphicsFamily.value();

    if (vkCreateCommandPool(device->GetLogical(), &poolInfo, nullptr, &m_Pool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create command pool!");
}

void CommandManager::Destroy()
{
    vkDestroyCommandPool(m_Device->GetLogical(), m_Pool, nullptr);
}

void CommandManager::AllocateCommandBuffers()
{
    m_Buffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_Pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_Buffers.size());

    if (vkAllocateCommandBuffers(m_Device->GetLogical(), &allocInfo, m_Buffers.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffers!");
}

VkCommandBuffer CommandManager::BeginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_Pool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(m_Device->GetLogical(), &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    return cmd;
}

void CommandManager::EndSingleTimeCommands(VkCommandBuffer cmd)
{
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_Device->GetGraphicsQueue());

    vkFreeCommandBuffers(m_Device->GetLogical(), m_Pool, 1, &cmd);
}
