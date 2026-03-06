
#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include<string>

class GraphicsPipeline
{

public:
    GraphicsPipeline() = default;
    void CreateGraphicsPipeline(VkDevice device,VkRenderPass RenderPass,VkDescriptorSetLayout desciptorSetLayOut);
    static std::vector<char> readFile(const std::string & filename);
    void DestroyPipelineLayout(VkDevice logicalDevice);
    void DestroyPipeline(VkDevice logicalDevice);

    VkPipeline GetPipeline();
    VkPipelineLayout GetpipelineLayout();

private:
    VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice device);
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

};