#include "Renderer.h"
#include <stdexcept>
#include <array>
#include <chrono>
#include <iostream>
#include <cmath>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// ---------------------------------------------------------------------------
// Point-light config table — single source of truth for both the LightUBO
// (UpdateUniformBuffer) and the emissive gizmo spheres (Init).
// position: world-space, Z-up. range in meters. intensity in lumens.
// ---------------------------------------------------------------------------
struct PointLightConfig
{
    glm::vec3 position;
    glm::vec3 color;
    float     intensity;
    float     range;
};

static const std::array<PointLightConfig, 8> kPointLights = { {
    {{-12.0f,  0.0f, 4.0f}, {1.00f, 0.45f, 0.15f}, 40000.0f, 14.0f}, // warm orange
    {{ -4.0f,  0.0f, 4.0f}, {1.00f, 0.15f, 0.15f}, 40000.0f, 14.0f}, // red
    {{  4.0f,  0.0f, 4.0f}, {1.00f, 0.95f, 0.30f}, 40000.0f, 14.0f}, // yellow
    {{ 12.0f,  0.0f, 4.0f}, {0.20f, 0.45f, 1.00f}, 40000.0f, 14.0f}, // cool blue
    {{ -8.0f, -6.0f, 4.0f}, {1.00f, 0.20f, 0.90f}, 30000.0f, 12.0f}, // magenta
    {{  8.0f, -6.0f, 4.0f}, {0.20f, 1.00f, 0.95f}, 30000.0f, 12.0f}, // cyan
    {{ -8.0f,  6.0f, 4.0f}, {0.30f, 1.00f, 0.35f}, 30000.0f, 12.0f}, // green
    {{  8.0f,  6.0f, 4.0f}, {0.75f, 0.30f, 1.00f}, 30000.0f, 12.0f}, // purple
} };

// Small unit-sphere gizmo — sized at draw time via model matrix scale.
static Mesh GenerateSphereMesh(int sectors = 24, int stacks = 16)
{
    Mesh m;
    for (int i = 0; i <= stacks; i++)
    {
        float v    = (float)i / stacks;
        float phi  = v * 3.14159265f;            // 0..pi
        float y    = std::cos(phi);
        float r    = std::sin(phi);
        for (int j = 0; j <= sectors; j++)
        {
            float u    = (float)j / sectors;
            float th   = u * 2.0f * 3.14159265f; // 0..2pi
            glm::vec3 p = { r * std::cos(th), y, r * std::sin(th) };
            Vertex vert{};
            vert.pos      = p;
            vert.normal   = glm::normalize(p);
            vert.color    = glm::vec3(1.0f);
            vert.texCoord = glm::vec2(u, v);
            vert.tangent  = glm::vec4(1, 0, 0, 1);
            m.vertices.push_back(vert);
        }
    }
    for (int i = 0; i < stacks; i++)
    {
        for (int j = 0; j < sectors; j++)
        {
            uint32_t a = i * (sectors + 1) + j;
            uint32_t b = a + sectors + 1;
            m.indices.push_back(a);     m.indices.push_back(b);     m.indices.push_back(a + 1);
            m.indices.push_back(a + 1); m.indices.push_back(b);     m.indices.push_back(b + 1);
        }
    }
    return m;
}

Renderer::Renderer() : m_Window(800, 600) {}
Renderer::~Renderer() {}

void Renderer::Run()
{
    Init();
    MainLoop();
    CleanUp();
}

void Renderer::Init()
{
    m_Window.Init();
    m_Instance.Create();
    m_Instance.SetupDebugMessenger();
    m_Window.CreateSurface(m_Instance.Get());
    m_Device.Init(&m_Instance, m_Window.GetSurface());
    m_SwapChain.Create(&m_Device, m_Window.GetSurface(), m_Window.GetHandle());
    m_SwapChain.CreateImageViews();
    m_CommandManager.Init(&m_Device);
    m_CommandManager.AllocateCommandBuffers();
    m_RenderPass.Create(&m_Device, m_SwapChain.GetFormat());

     // Load Sponza (only)
    RenderableScene sponza;
    std::string sponzaPath = std::string(PROJECT_SOURCE_DIR) + "/Models/Sponza/Sponza.gltf";
    sponza.scene = GltfLoader::Load(sponzaPath, &m_Device, &m_CommandManager);
    sponza.meshes.resize(sponza.scene.meshes.size());
    for (int i = 0; i < (int)sponza.scene.meshes.size(); i++)
        sponza.meshes[i].UploadMesh(&m_Device, &m_CommandManager, sponza.scene.meshes[i].mesh);
    sponza.materialOffset = 0;
    m_Scenes.push_back(std::move(sponza));

    // -------------------------------------------------------
    // Light gizmos — one emissive sphere per point light.
    // Each gizmo is its own RenderableScene with one mesh + one material so
    // it plugs into the existing per-scene draw / model-matrix loop.
    // The emissive path in geometry.frag/lighting.frag bypasses BRDF, so these
    // spheres glow at their light color regardless of scene illumination.
    // -------------------------------------------------------
    Mesh sphereMesh = GenerateSphereMesh();
    int gizmoMatOffset = (int)m_Scenes.back().scene.materials.size(); // after Sponza
    for (const auto& lc : kPointLights)
    {
        RenderableScene gizmo;
        GltfMesh gm; gm.mesh = sphereMesh; gm.materialIndex = 0;
        gizmo.scene.meshes.push_back(gm);

        Material gizmoMat{};
        // Emissive-only: BRDF path will still run but albedo=black => no diffuse/spec.
        gizmoMat.baseColorFactor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        gizmoMat.emissiveFactor  = lc.color * 40.0f; // bright enough to read through tone map
        gizmoMat.roughnessFactor = 1.0f;
        gizmoMat.metallicFactor  = 0.0f;
        gizmo.scene.materials.push_back(gizmoMat);

        gizmo.meshes.resize(1);
        gizmo.meshes[0].UploadMesh(&m_Device, &m_CommandManager, sphereMesh);
        gizmo.materialOffset = gizmoMatOffset++;
        m_Scenes.push_back(std::move(gizmo));
    }

    // IBL — load HDR environment map and precompute irradiance cubemap
    std::string hdrPath = std::string(PROJECT_SOURCE_DIR) + "/Textures/environment.hdr";
    m_IBL.Create(&m_Device, &m_CommandManager, hdrPath);

    // Shadow map
    m_ShadowMap.Create(&m_Device, &m_CommandManager);



    // Fallback white texture
    m_FallbackTexture.Load(&m_Device, &m_CommandManager,
        std::string(PROJECT_SOURCE_DIR) + "/Textures/white.png");

    // Uniform buffers
    m_UniformBuffer.Create(&m_Device, CommandManager::MAX_FRAMES_IN_FLIGHT);

    // Collect all materials from all scenes
    int totalMatCount = 0;
    for (auto& rs : m_Scenes)
        totalMatCount += (int)rs.scene.materials.size();

    m_UniformBuffer.CreateMaterialBuffers(&m_Device, totalMatCount);

    std::vector<VkImageView> albedoViews(totalMatCount), normalViews(totalMatCount);
    std::vector<VkSampler>   albedoSamplers(totalMatCount), normalSamplers(totalMatCount);
    std::vector<VkImageView> mrViews(totalMatCount), aoViews(totalMatCount), emissiveViews(totalMatCount);
    std::vector<VkSampler>   mrSamplers(totalMatCount), aoSamplers(totalMatCount), emissiveSamplers(totalMatCount);
    std::vector<VkBuffer>    matBuffers(totalMatCount);

    int globalMatIdx = 0;
    for (auto& rs : m_Scenes)
    {
        for (int i = 0; i < (int)rs.scene.materials.size(); i++, globalMatIdx++)
        {
            auto& mat = rs.scene.materials[i];

            MaterialUBO mubo{};
            mubo.baseColorFactor = mat.baseColorFactor;
            mubo.emissiveFactor = glm::vec4(mat.emissiveFactor, 0.0f);
            mubo.metallicFactor = mat.metallicFactor;
            mubo.roughnessFactor = mat.roughnessFactor;
            mubo.hasNormalMap = mat.hasNormalMap ? 1.0f : 0.0f;
            mubo.hasMetallicRoughness = mat.hasMetallicRoughness ? 1.0f : 0.0f;
            mubo.hasAO = mat.hasAO ? 1.0f : 0.0f;
            mubo.hasEmissive = mat.hasEmissive ? 1.0f : 0.0f;
            mubo.alphaMask = mat.alphaMask ? 1.0f : 0.0f;
            mubo.alphaCutoff = mat.alphaCutoff;
            m_UniformBuffer.UpdateMaterial(globalMatIdx, mubo);

            albedoViews[globalMatIdx] = mat.albedoView;
            albedoSamplers[globalMatIdx] = mat.albedoSampler;
            normalViews[globalMatIdx] = mat.normalView;
            normalSamplers[globalMatIdx] = mat.normalSampler;
            mrViews[globalMatIdx] = mat.metallicRoughnessView;
            mrSamplers[globalMatIdx] = mat.metallicRoughnessSampler;
            aoViews[globalMatIdx] = mat.aoView;
            aoSamplers[globalMatIdx] = mat.aoSampler;
            emissiveViews[globalMatIdx] = mat.emissiveView;
            emissiveSamplers[globalMatIdx] = mat.emissiveSampler;
            matBuffers[globalMatIdx] = m_UniformBuffer.GetMaterialBuffer(globalMatIdx);
        }
    }

    // Camera descriptors (set 0)
    m_Descriptors.CreateCameraLayout(&m_Device);
    m_Descriptors.CreateCameraPool(&m_Device, CommandManager::MAX_FRAMES_IN_FLIGHT);
    std::vector<VkBuffer> cameraBuffers(CommandManager::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < CommandManager::MAX_FRAMES_IN_FLIGHT; i++)
        cameraBuffers[i] = m_UniformBuffer.GetBuffer(i);
    m_Descriptors.CreateCameraSets(&m_Device, CommandManager::MAX_FRAMES_IN_FLIGHT, cameraBuffers);

    // Material descriptors (set 1)
    m_Descriptors.CreateMaterialLayout(&m_Device);
    m_Descriptors.CreateMaterialPool(&m_Device, totalMatCount);
    m_Descriptors.CreateMaterialSets(&m_Device, totalMatCount, matBuffers,
        albedoViews, albedoSamplers,
        normalViews, normalSamplers,
        mrViews, mrSamplers,
        aoViews, aoSamplers,
        emissiveViews, emissiveSamplers,
        m_FallbackTexture.GetView(), m_FallbackTexture.GetSampler());

    // Lighting layout
    m_Descriptors.CreateLightingLayout(&m_Device);

    // Tone mapping layout
    m_Descriptors.CreateToneMappingLayout(&m_Device);

    // Pipelines
    m_DepthPrepassPipeline.CreateDepthPrepass(&m_Device, m_RenderPass.Get(),
        m_Descriptors.GetCameraLayout(), m_Descriptors.GetMaterialLayout());
    m_GeometryPipeline.CreateGeometry(&m_Device, m_RenderPass.Get(),
        m_Descriptors.GetCameraLayout(), m_Descriptors.GetMaterialLayout());
    m_LightingPipeline.CreateLighting(&m_Device, m_RenderPass.Get(),
        m_Descriptors.GetLightingLayout());
    m_ToneMappingPipeline.CreateToneMapping(&m_Device, m_RenderPass.Get(),
        m_Descriptors.GetToneMappingLayout());
    m_ShadowPipeline.CreateShadow(&m_Device, m_ShadowMap.GetRenderPass());

    CreateSwapChainDependents();
    CreateSyncObjects();

    float aspect = m_SwapChain.GetExtent().width / (float)m_SwapChain.GetExtent().height;
    m_Camera.Init(m_Window.GetHandle(), 45.0f, aspect, 0.1f, 5000.0f);
}

void Renderer::CreateSwapChainDependents()
{
    VkExtent2D extent = m_SwapChain.GetExtent();

    m_DepthBuffer.Create(&m_Device, extent);
    m_GBuffer.Create(&m_Device, extent);
    m_HDRBuffer.Create(&m_Device, extent);

    std::vector<VkImageView> gbufferViews(GBuffer::Count);
    for (int i = 0; i < GBuffer::Count; i++)
        gbufferViews[i] = m_GBuffer.GetView(i);

    m_FrameBuffer.Create(&m_Device, m_RenderPass.Get(),
        m_SwapChain.GetImageViews(),
        m_DepthBuffer.GetView(),
        gbufferViews,
        m_HDRBuffer.GetView(),
        extent);

    std::vector<VkBuffer> lightBuffers(CommandManager::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < CommandManager::MAX_FRAMES_IN_FLIGHT; i++)
        lightBuffers[i] = m_UniformBuffer.GetLightBuffer(i);

    std::array<VkImageView, 5> gbufferArr = {
        m_GBuffer.GetView(0), m_GBuffer.GetView(1),
        m_GBuffer.GetView(2), m_GBuffer.GetView(3),
        m_GBuffer.GetView(4)
    };
    m_Descriptors.CreateLightingPool(&m_Device);
    m_Descriptors.CreateLightingSet(&m_Device, gbufferArr, lightBuffers,
        CommandManager::MAX_FRAMES_IN_FLIGHT,
        m_ShadowMap.GetView(),        m_ShadowMap.GetSampler(),
        m_IBL.GetIrradianceView(),    m_IBL.GetIrradianceSampler(),
        m_IBL.GetPrefilterView(),     m_IBL.GetPrefilterSampler(),
        m_IBL.GetBrdfLutView(),       m_IBL.GetBrdfLutSampler());

    m_Descriptors.CreateToneMappingPool(&m_Device);
    m_Descriptors.CreateToneMappingSet(&m_Device, m_HDRBuffer.GetView(),
        lightBuffers, CommandManager::MAX_FRAMES_IN_FLIGHT);
}

void Renderer::DestroySwapChainDependents()
{
    m_Descriptors.DestroyToneMappingPool();
    m_Descriptors.DestroyLightingPool();
    m_FrameBuffer.Destroy();
    m_HDRBuffer.Destroy();
    m_GBuffer.Destroy();
    m_DepthBuffer.Destroy();
}

void Renderer::RecreateSwapChain()
{
    int w = 0, h = 0;
    while (w == 0 || h == 0)
    {
        m_Window.GetFramebufferSize(w, h);
        m_Window.WaitEvents();
    }
    vkDeviceWaitIdle(m_Device.GetLogical());
    DestroySwapChainDependents();
    m_SwapChain.DestroyImageViews();
    m_SwapChain.Destroy();
    m_SwapChain.Create(&m_Device, m_Window.GetSurface(), m_Window.GetHandle());
    m_SwapChain.CreateImageViews();
    CreateSwapChainDependents();
    float aspect = m_SwapChain.GetExtent().width / (float)m_SwapChain.GetExtent().height;
    m_Camera.OnResize(aspect);
}

void Renderer::MainLoop()
{
    auto lastTime = std::chrono::high_resolution_clock::now();
    while (!m_Window.ShouldClose())
    {
        auto now = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        m_Window.PollEvents();
        m_Camera.Update(deltaTime);
        DrawFrame();
    }
    vkDeviceWaitIdle(m_Device.GetLogical());
}

void Renderer::DrawFrame()
{
    vkWaitForFences(m_Device.GetLogical(), 1, &m_InFlight[m_CurrentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(m_Device.GetLogical(), m_SwapChain.Get(),
        UINT64_MAX, m_ImageAvailable[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) { RecreateSwapChain(); return; }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        throw std::runtime_error("Failed to acquire swap chain image!");

    UpdateUniformBuffer(m_CurrentFrame);
    vkResetFences(m_Device.GetLogical(), 1, &m_InFlight[m_CurrentFrame]);

    VkCommandBuffer cmd = m_CommandManager.GetBuffer(m_CurrentFrame);
    vkResetCommandBuffer(cmd, 0);
    RecordCommandBuffer(cmd, imageIndex);

    VkSemaphore waitSemaphores[] = { m_ImageAvailable[m_CurrentFrame] };
    VkSemaphore signalSemaphores[] = { m_RenderFinished[m_CurrentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_Device.GetGraphicsQueue(), 1, &submitInfo, m_InFlight[m_CurrentFrame]) != VK_SUCCESS)
        throw std::runtime_error("Failed to submit draw command buffer!");

    VkSwapchainKHR swapChains[] = { m_SwapChain.Get() };
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(m_Device.GetPresentQueue(), &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_Window.FramebufferResized)
    {
        m_Window.FramebufferResized = false;
        RecreateSwapChain();
    }
    else if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to present swap chain image!");

    m_CurrentFrame = (m_CurrentFrame + 1) % CommandManager::MAX_FRAMES_IN_FLIGHT;
}

void Renderer::RecordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &beginInfo);

    std::array<VkClearValue, 8> clearValues{};
    clearValues[0].color = { {0.529f, 0.808f, 0.922f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };
    clearValues[2].color = { {0.0f, 0.0f, 0.0f, 0.0f} };
    clearValues[3].color = { {0.0f, 0.0f, 0.0f, 0.0f} };
    clearValues[4].color = { {0.0f, 0.0f, 0.0f, 0.0f} };
    clearValues[5].color = { {0.0f, 0.0f, 0.0f, 0.0f} };
    clearValues[6].color = { {0.0f, 0.0f, 0.0f, 0.0f} };
    clearValues[7].color = { {0.0f, 0.0f, 0.0f, 0.0f} };

    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = m_RenderPass.Get();
    rpInfo.framebuffer = m_FrameBuffer.Get()[imageIndex];
    rpInfo.renderArea.offset = { 0, 0 };
    rpInfo.renderArea.extent = m_SwapChain.GetExtent();
    rpInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    rpInfo.pClearValues = clearValues.data();

    // Model matrices (one per scene).
    // [0] Sponza (glTF Y-up → Z-up). [1..N] light gizmo spheres at point-light positions.
    glm::mat4 sponzaModel = glm::rotate(glm::mat4(1.0f),
        glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    std::vector<glm::mat4> modelMatrices;
    modelMatrices.reserve(1 + kPointLights.size());
    modelMatrices.push_back(sponzaModel);
    for (const auto& lc : kPointLights)
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), lc.position);
        m = glm::scale(m, glm::vec3(0.15f)); // 15cm radius marker
        modelMatrices.push_back(m);
    }

    // -------------------------------------------------------
    // Shadow pass — render depth from the light's POV
    // -------------------------------------------------------
    {
        VkClearValue shadowClear{};
        shadowClear.depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo shadowRP{};
        shadowRP.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        shadowRP.renderPass        = m_ShadowMap.GetRenderPass();
        shadowRP.framebuffer       = m_ShadowMap.GetFramebuffer();
        shadowRP.renderArea.offset = { 0, 0 };
        shadowRP.renderArea.extent = { ShadowMap::SIZE, ShadowMap::SIZE };
        shadowRP.clearValueCount   = 1;
        shadowRP.pClearValues      = &shadowClear;

        vkCmdBeginRenderPass(cmd, &shadowRP, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport sv{};
        sv.x = 0; sv.y = 0;
        sv.width  = static_cast<float>(ShadowMap::SIZE);
        sv.height = static_cast<float>(ShadowMap::SIZE);
        sv.minDepth = 0.0f; sv.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &sv);
        VkRect2D ss = { {0, 0}, {ShadowMap::SIZE, ShadowMap::SIZE} };
        vkCmdSetScissor(cmd, 0, 1, &ss);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ShadowPipeline.Get());

        // Push lightSpaceMatrix at offset 0
        vkCmdPushConstants(cmd, m_ShadowPipeline.GetLayout(),
            VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &m_LightSpaceMatrix);

        for (int s = 0; s < (int)m_Scenes.size(); s++)
        {
            auto& rs = m_Scenes[s];
            // Push model matrix at offset 64
            vkCmdPushConstants(cmd, m_ShadowPipeline.GetLayout(),
                VK_SHADER_STAGE_VERTEX_BIT, 64, sizeof(glm::mat4), &modelMatrices[s]);
            for (int i = 0; i < (int)rs.meshes.size(); i++)
            {
                VkBuffer     vb[]  = { rs.meshes[i].GetVertexBuffer() };
                VkDeviceSize off[] = { 0 };
                vkCmdBindVertexBuffers(cmd, 0, 1, vb, off);
                vkCmdBindIndexBuffer(cmd, rs.meshes[i].GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(cmd, rs.meshes[i].GetIndexCount(), 1, 0, 0, 0);
            }
        }

        vkCmdEndRenderPass(cmd);
    }

    // -------------------------------------------------------
    // Main deferred render pass
    // -------------------------------------------------------
    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f; viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_SwapChain.GetExtent().width);
    viewport.height = static_cast<float>(m_SwapChain.GetExtent().height);
    viewport.minDepth = 0.0f; viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{ {0, 0}, m_SwapChain.GetExtent() };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    VkDescriptorSet camSet = m_Descriptors.GetCameraSet(m_CurrentFrame);

    // --- Subpass 0: Depth Prepass ---
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_DepthPrepassPipeline.Get());
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_DepthPrepassPipeline.GetLayout(), 0, 1, &camSet, 0, nullptr);

    for (int s = 0; s < (int)m_Scenes.size(); s++)
    {
        auto& rs = m_Scenes[s];
        vkCmdPushConstants(cmd, m_DepthPrepassPipeline.GetLayout(),
            VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &modelMatrices[s]);

        for (int i = 0; i < (int)rs.meshes.size(); i++)
        {
            int matIdx = rs.scene.meshes[i].materialIndex;
            if (matIdx < 0) matIdx = 0;
            matIdx += rs.materialOffset;
            VkDescriptorSet matSet = m_Descriptors.GetMaterialSet(matIdx);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_DepthPrepassPipeline.GetLayout(), 1, 1, &matSet, 0, nullptr);
            VkBuffer vb[] = { rs.meshes[i].GetVertexBuffer() };
            VkDeviceSize off[] = { 0 };
            vkCmdBindVertexBuffers(cmd, 0, 1, vb, off);
            vkCmdBindIndexBuffer(cmd, rs.meshes[i].GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(cmd, rs.meshes[i].GetIndexCount(), 1, 0, 0, 0);
        }
    }

    // --- Subpass 1: Geometry Pass ---
    vkCmdNextSubpass(cmd, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GeometryPipeline.Get());
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_GeometryPipeline.GetLayout(), 0, 1, &camSet, 0, nullptr);

    struct GeomPC { glm::mat4 model; glm::mat4 normalMatrix; };

    for (int s = 0; s < (int)m_Scenes.size(); s++)
    {
        auto& rs = m_Scenes[s];
        GeomPC gpc;
        gpc.model = modelMatrices[s];
        glm::mat3 nm3 = glm::transpose(glm::inverse(glm::mat3(modelMatrices[s])));
        gpc.normalMatrix = glm::mat4(nm3);
        vkCmdPushConstants(cmd, m_GeometryPipeline.GetLayout(),
            VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GeomPC), &gpc);

        for (int i = 0; i < (int)rs.meshes.size(); i++)
        {
            int matIdx = rs.scene.meshes[i].materialIndex;
            if (matIdx < 0) matIdx = 0;
            matIdx += rs.materialOffset;
            VkDescriptorSet matSet = m_Descriptors.GetMaterialSet(matIdx);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_GeometryPipeline.GetLayout(), 1, 1, &matSet, 0, nullptr);
            VkBuffer vb[] = { rs.meshes[i].GetVertexBuffer() };
            VkDeviceSize off[] = { 0 };
            vkCmdBindVertexBuffers(cmd, 0, 1, vb, off);
            vkCmdBindIndexBuffer(cmd, rs.meshes[i].GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(cmd, rs.meshes[i].GetIndexCount(), 1, 0, 0, 0);
        }
    }

    // --- Subpass 2: Lighting Pass ---
    vkCmdNextSubpass(cmd, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_LightingPipeline.Get());
    VkDescriptorSet lightSet = m_Descriptors.GetLightingSet(m_CurrentFrame);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_LightingPipeline.GetLayout(), 0, 1, &lightSet, 0, nullptr);
    vkCmdDraw(cmd, 3, 1, 0, 0);

    // --- Subpass 3: Tone Mapping ---
    vkCmdNextSubpass(cmd, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ToneMappingPipeline.Get());
    VkDescriptorSet tmSet = m_Descriptors.GetToneMappingSet(m_CurrentFrame);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_ToneMappingPipeline.GetLayout(), 0, 1, &tmSet, 0, nullptr);
    vkCmdDraw(cmd, 3, 1, 0, 0);

    vkCmdEndRenderPass(cmd);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
        throw std::runtime_error("Failed to record command buffer!");
}

void Renderer::UpdateUniformBuffer(uint32_t frame)
{
    UniformBufferObject ubo{};
    ubo.view = m_Camera.GetView();
    ubo.proj = m_Camera.GetProjection();
    ubo.model = glm::mat4(1.0f); // model now handled by push constants
    m_UniformBuffer.Update(frame, ubo);

    LightUBO light{};

    // -------------------------------------------------------
    // Directional sun — angled down in Z-up world
    // -------------------------------------------------------
    glm::vec3 sunDir = glm::normalize(glm::vec3(0.4f, 0.3f, -1.0f));
    light.dirLightDir   = glm::vec4(sunDir, 0.0f);
    light.dirLightColor = glm::vec4(1.0f, 0.95f, 0.8f, 30000.0f);  // warm afternoon, 30 klux
    light.camPos        = glm::vec4(m_Camera.GetPosition(), 0.0f);

    // Sky ambient — soft blue hemisphere (only used when IBL is disabled)
    light.skyLight = glm::vec4(0.53f, 0.73f, 1.0f, 5000.0f);  // 5 klux hemisphere

    // Physical camera — daylight setting (ISO 100, f/8, 1/125s ~ outdoor sunny)
    light.aperture     = 8.0f;
    light.shutterSpeed = 1.0f / 125.0f;
    light.iso          = 100.0f;

    // IBL — sample the HDR cubemap at its actual radiance; no artificial boost.
    light.useIBL       = m_IBL.IsValid() ? 1 : 0;
    light.iblIntensity = 1.0f;

    // -------------------------------------------------------
    // Light-space matrix for shadow map
    // Orthographic frustum encompassing the Sponza hall (Z-up world)
    // -------------------------------------------------------
    // Frustum sized for Khronos Sponza (~30m wide x 13m long x 16m tall).
    // Wider XY range so balconies/sides are inside the shadow map; longer
    // depth range so the bottom floor isn't past the far plane.
    glm::vec3 lightPos  = -sunDir * 40.0f;
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 lightProj = glm::ortho(-25.0f, 25.0f, -25.0f, 25.0f, 0.1f, 120.0f);

    // Vulkan NDC: Y points down, depth [0,1].
    // Apply scale-bias to map clip Z from [-1,1] → [0,1] so depth comparisons work.
    // Y-flip is baked in via lightProj[1][1] *= -1.
    lightProj[1][1] *= -1.0f;
    glm::mat4 vulkanZ(1.0f);
    vulkanZ[2][2] = 0.5f;
    vulkanZ[3][2] = 0.5f;
    m_LightSpaceMatrix         = vulkanZ * lightProj * lightView;
    light.lightSpaceMatrix     = m_LightSpaceMatrix;

    // -------------------------------------------------------
    // Spot lights — warm lanterns along the hall (Z-up)
    // -------------------------------------------------------
    light.spotLightCount = 4;

    auto& s0 = light.spotLights[0];
    s0.position  = glm::vec4( 0.0f,  0.0f, 9.5f, 20.0f);
    s0.direction = glm::vec4(glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f)), glm::cos(glm::radians(25.0f)));
    s0.color     = glm::vec4(1.0f, 0.85f, 0.6f, 80000.0f);
    s0.outerCone = glm::cos(glm::radians(40.0f));

    auto& s1 = light.spotLights[1];
    s1.position  = glm::vec4(12.0f,  0.0f, 9.0f, 20.0f);
    s1.direction = glm::vec4(glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f)), glm::cos(glm::radians(25.0f)));
    s1.color     = glm::vec4(1.0f, 0.85f, 0.6f, 80000.0f);
    s1.outerCone = glm::cos(glm::radians(40.0f));

    auto& s2 = light.spotLights[2];
    s2.position  = glm::vec4(-12.0f, 0.0f, 9.0f, 20.0f);
    s2.direction = glm::vec4(glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f)), glm::cos(glm::radians(25.0f)));
    s2.color     = glm::vec4(1.0f, 0.85f, 0.6f, 80000.0f);
    s2.outerCone = glm::cos(glm::radians(40.0f));

    auto& s3 = light.spotLights[3];
    s3.position  = glm::vec4( 0.0f,  6.0f, 7.0f, 15.0f);
    s3.direction = glm::vec4(glm::normalize(glm::vec3(0.0f, -0.3f, -1.0f)), glm::cos(glm::radians(30.0f)));
    s3.color     = glm::vec4(1.0f, 0.9f, 0.7f, 60000.0f);
    s3.outerCone = glm::cos(glm::radians(50.0f));

    // -------------------------------------------------------
    // Point lights — driven from kPointLights (shared with gizmo spheres)
    // -------------------------------------------------------
    light.pointLightCount = (int)kPointLights.size();
    for (int i = 0; i < (int)kPointLights.size(); i++)
    {
        const auto& lc = kPointLights[i];
        light.pointLights[i].position = glm::vec4(lc.position, lc.range);
        light.pointLights[i].color    = glm::vec4(lc.color,    lc.intensity);
    }

    m_UniformBuffer.UpdateLight(frame, light);
}

void Renderer::CreateSyncObjects()
{
    m_ImageAvailable.resize(CommandManager::MAX_FRAMES_IN_FLIGHT);
    m_RenderFinished.resize(CommandManager::MAX_FRAMES_IN_FLIGHT);
    m_InFlight.resize(CommandManager::MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < CommandManager::MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(m_Device.GetLogical(), &semInfo, nullptr, &m_ImageAvailable[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_Device.GetLogical(), &semInfo, nullptr, &m_RenderFinished[i]) != VK_SUCCESS ||
            vkCreateFence(m_Device.GetLogical(), &fenceInfo, nullptr, &m_InFlight[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create sync objects!");
    }
}

void Renderer::DestroySyncObjects()
{
    for (int i = 0; i < CommandManager::MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(m_Device.GetLogical(), m_RenderFinished[i], nullptr);
        vkDestroySemaphore(m_Device.GetLogical(), m_ImageAvailable[i], nullptr);
        vkDestroyFence(m_Device.GetLogical(), m_InFlight[i], nullptr);
    }
}

void Renderer::CleanUp()
{
    DestroySwapChainDependents();
    m_SwapChain.DestroyImageViews();
    m_SwapChain.Destroy();

    for (auto& rs : m_Scenes)
    {
        rs.scene.Destroy(m_Device.GetLogical());
        for (auto& mesh : rs.meshes) mesh.Destroy();
    }
    m_FallbackTexture.Destroy();
    m_UniformBuffer.Destroy(CommandManager::MAX_FRAMES_IN_FLIGHT);

    m_Descriptors.DestroyCameraPool();
    m_Descriptors.DestroyCameraLayout();
    m_Descriptors.DestroyMaterialPool();
    m_Descriptors.DestroyMaterialLayout();
    m_Descriptors.DestroyLightingLayout();
    m_Descriptors.DestroyToneMappingLayout();

    m_DepthPrepassPipeline.Destroy();
    m_GeometryPipeline.Destroy();
    m_LightingPipeline.Destroy();
    m_ToneMappingPipeline.Destroy();
    m_ShadowPipeline.Destroy();
    m_RenderPass.Destroy();
    m_ShadowMap.Destroy();
    m_IBL.Destroy();

    DestroySyncObjects();
    m_CommandManager.Destroy();
    m_Device.Destroy();
    m_Instance.DestroyDebugMessenger();
    m_Window.DestroySurface(m_Instance.Get());
    m_Instance.Destroy();
    m_Window.Destroy();
}