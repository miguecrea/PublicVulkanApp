#pragma once
#include <vulkan/vulkan.h>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
#include "Rendering/GBuffer.h"
#include "Resources/DepthBuffer.h"
#include "Resources/Texture.h"
#include "Resources/Buffer.h"
#include "Resources/ModelLoader.h"
#include "Scene/Camera.h"
#include "Resources/GltfLoader.h"
#include "Resources/Material.h"
#include "Rendering/HDRBuffer.h"
#include "Rendering/ShadowMap.h"
#include "Rendering/IBL.h"



class Renderer
{
public:
    Renderer();
    ~Renderer();

    void Run();

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
    UniformBuffer m_UniformBuffer;

    // Pipelines (one per subpass)
    Pipeline m_DepthPrepassPipeline;
    Pipeline m_GeometryPipeline;
    Pipeline m_LightingPipeline;

    // Descriptors (one set per pass)
    Descriptors m_Descriptors;

    // G-Buffer
    GBuffer m_GBuffer;

    // Resources
    DepthBuffer m_DepthBuffer;
    Texture     m_Texture;
    Buffer      m_Buffer;




 
    struct RenderableScene
    {
        GltfScene           scene;
        std::vector<Buffer> meshes;
        int                 materialOffset = 0; // offset into global material array
    };

    std::vector<RenderableScene> m_Scenes;
    Texture                m_FallbackTexture; // white 1x1 for missing textures


    //HDR
    HDRBuffer m_HDRBuffer;
    Pipeline  m_ToneMappingPipeline;

    // Shadow mapping
    ShadowMap m_ShadowMap;
    Pipeline  m_ShadowPipeline;
    glm::mat4 m_LightSpaceMatrix{1.0f};

    // Image-Based Lighting
    IBL m_IBL;



    // Scene
    Camera m_Camera;

    // Sync
    std::vector<VkSemaphore> m_ImageAvailable;
    std::vector<VkSemaphore> m_RenderFinished;
    std::vector<VkFence>     m_InFlight;

    uint32_t m_CurrentFrame = 0;
};