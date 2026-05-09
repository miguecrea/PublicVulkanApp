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

    // Shadow pass — depth only, no descriptor sets, 128-byte push constants
    // (offset 0 = lightSpaceMatrix mat4, offset 64 = model mat4)
    void CreateShadow(Device* device, VkRenderPass shadowRenderPass);

private:
    Device* m_Device = nullptr;
    VkPipeline       m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_Layout = VK_NULL_HANDLE;

    static std::vector<char> ReadFile(const std::string& path);
    VkShaderModule CreateShaderModule(const std::vector<char>& code);
    VkPipelineShaderStageCreateInfo MakeStage(VkShaderStageFlagBits stage, VkShaderModule module);

    void BuildLayout(VkDescriptorSetLayout cameraLayout, VkDescriptorSetLayout materialLayout, uint32_t pushSize = sizeof(float) * 16);
    void BuildLayoutSingle(VkDescriptorSetLayout layout);
};