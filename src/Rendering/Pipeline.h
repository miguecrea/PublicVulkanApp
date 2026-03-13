#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

class Device;

class Pipeline
{
public:
    Pipeline() = default;
    ~Pipeline() = default;

    void Create(Device* device, VkRenderPass renderPass, VkDescriptorSetLayout descriptorLayout);
    void Destroy();

    VkPipeline Get() const { return m_Pipeline; }
    VkPipelineLayout GetLayout() const { return m_Layout; }

private:
    Device* m_Device = nullptr;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_Layout = VK_NULL_HANDLE;

    static std::vector<char> ReadFile(const std::string& path);
    VkShaderModule CreateShaderModule(const std::vector<char>& code);
};
