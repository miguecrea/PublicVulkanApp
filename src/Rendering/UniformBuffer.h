#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <glm/glm.hpp>

class Device;

// Matches what's sent to the vertex shader
struct UniformBufferObject
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

class UniformBuffer
{
public:
    UniformBuffer() = default;
    ~UniformBuffer() = default;

    void Create(Device* device, int framesInFlight);
    void Destroy(int framesInFlight);

    void Update(uint32_t frame, const UniformBufferObject& ubo);

    VkBuffer GetBuffer(int frame) const { return m_Buffers[frame]; }

private:
    Device* m_Device = nullptr;
    std::vector<VkBuffer> m_Buffers;
    std::vector<VkDeviceMemory> m_Memory;
    std::vector<void*> m_Mapped;
};
