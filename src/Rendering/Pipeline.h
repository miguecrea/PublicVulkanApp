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

    void CreateDepthPrepass(Device* device, VkRenderPass renderPass,
        VkDescriptorSetLayout cameraLayout, VkDescriptorSetLayout materialLayout);
    void CreateGeometry(Device* device, VkRenderPass renderPass,
        VkDescriptorSetLayout cameraLayout, VkDescriptorSetLayout materialLayout);
    void CreateLighting(Device* device, VkRenderPass renderPass,
        VkDescriptorSetLayout lightingLayout);

    void Destroy();

    VkPipeline       Get()       const { return m_Pipeline; }
    VkPipelineLayout GetLayout() const { return m_Layout; }


    void CreateToneMapping(Device * device, VkRenderPass renderPass,
        VkDescriptorSetLayout toneMappingLayout);

private:
    Device* m_Device = nullptr;
    VkPipeline       m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_Layout = VK_NULL_HANDLE;

    static std::vector<char> ReadFile(const std::string& path);
    VkShaderModule CreateShaderModule(const std::vector<char>& code);
    VkPipelineShaderStageCreateInfo MakeStage(VkShaderStageFlagBits stage, VkShaderModule module);

    void BuildLayout(VkDescriptorSetLayout cameraLayout, VkDescriptorSetLayout materialLayout);
    void BuildLayoutSingle(VkDescriptorSetLayout layout);
};