
#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include<string>

class GraphicsPipeline
{

public:
    GraphicsPipeline() = default;
    void CreateGraphicsPipeline(VkDevice device,VkRenderPass RenderPass);
    static std::vector<char> readFile(const std::string & filename);
    void DestroyPipelineLayout(VkDevice logicalDevice);
    void DestroyPipeline(VkDevice logicalDevice);

    VkPipeline GetPipeline();
private:
    VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice device);
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

};