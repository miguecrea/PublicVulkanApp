#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

#include "Core/Window.h"
#include "Core/Instance.h"
#include "Core/Device.h"
#include "Core/SwapChain.h"
#include "Core/CommandManager.h"
#include "Rendering/RenderPass.h"
#include "Rendering/FrameBuffer.h"
#include "Rendering/Pipeline.h"
#include "Rendering/Descriptors.h"
#include "Rendering/UniformBuffer.h"
#include "Resources/DepthBuffer.h"
#include "Resources/Texture.h"
#include "Resources/Buffer.h"
#include "Resources/ModelLoader.h"
#include "Scene/Camera.h"

class Renderer
{
public:
    Renderer();
    ~Renderer();

    void Run();

    // Exposed for GLFW resize callback
    bool FramebufferResized = false;

private:
    void Init();
    void MainLoop();
    void DrawFrame();
    void CleanUp();

    void CreateSwapChainDependents();
    void DestroySwapChainDependents();
    void RecreateSwapChain();

    void CreateSyncObjects();
    void DestroySyncObjects();

    void RecordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex);
    void UpdateUniformBuffer(uint32_t frame);

    // Core
    Window         m_Window;
    Instance       m_Instance;
    Device         m_Device;
    SwapChain      m_SwapChain;
    CommandManager m_CommandManager;

    // Rendering
    RenderPass    m_RenderPass;
    FrameBuffer   m_FrameBuffer;
    Pipeline      m_Pipeline;
    Descriptors   m_Descriptors;
    UniformBuffer m_UniformBuffer;

    // Resources
    DepthBuffer m_DepthBuffer;
    Texture     m_Texture;
    Buffer      m_Buffer;

    // Scene
    Camera m_Camera;

    // Sync
    std::vector<VkSemaphore> m_ImageAvailable;
    std::vector<VkSemaphore> m_RenderFinished;
    std::vector<VkFence>     m_InFlight;

    uint32_t m_CurrentFrame = 0;
};
