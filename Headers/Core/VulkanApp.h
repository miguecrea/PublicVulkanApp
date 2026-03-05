#pragma once

#include "../Headers/Core/VulkanApp.h"
#include <vulkan/vulkan.h>

class Window;
class InstanceManager;
class DeviceManager;
class SwapChain;
class GraphicsPipeline;
class RenderPass;
class FramebufferManager;
class CommandManager;

class Renderer final
{
public:
    Renderer();
    ~Renderer();
    void Run();

    void DrawFrame();

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

    void createSemaphoresObjects(VkDevice device);
    void DestroySemaphoresObjects(VkDevice device);

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
    void  setupDebugMessenger();
};


