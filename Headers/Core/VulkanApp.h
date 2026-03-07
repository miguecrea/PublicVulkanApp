#pragma once

#include "../Headers/Core/VulkanApp.h"
#include <vulkan/vulkan.h>
#include<vector>
#include"memory.h"
class Window;
class InstanceManager;
class DeviceManager;
class SwapChain;
class GraphicsPipeline;
class RenderPass;
class FramebufferManager;
class CommandManager;
class BufferManager;
class DescriptorSetLayout;
class Image;

class Renderer final
{
public:
    Renderer();
    ~Renderer();
    void Run();

    void DrawFrame();
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;

    bool framebufferResized = false;

    void createSemaphoresObjects(VkDevice device);
    void DestroySemaphoresObjects(VkDevice device);

    void RecreateSwapChain();
    void CleanUpSwapChain();







private:
    void InitWindow();
    void InitVulkan();
    void MainLoop();
    void CleanUp();

    Window * m_Window;
    InstanceManager * m_InstanceManager;
    DeviceManager * m_DeviceManager;
    SwapChain * m_SwapChain;
    GraphicsPipeline * m_GraphicsPipeline;
    RenderPass * m_RenderPass;
    FramebufferManager * m_FrameBuffer;
    CommandManager * m_CommandManager;
    BufferManager * m_vertexBuffer;
    DescriptorSetLayout * m_DescriptorSetsLayout;
    Image * m_ImageManager;
    void  setupDebugMessenger();
};


