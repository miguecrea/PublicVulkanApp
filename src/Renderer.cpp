#include "Renderer.h"
#include <stdexcept>
#include <iostream>
#include <array>
#include <chrono>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Renderer::Renderer()
    : m_Window(800, 600)
{
}

Renderer::~Renderer()
{
}

void Renderer::Run()
{
    Init();
    MainLoop();
    CleanUp();
}

void Renderer::Init()
{
    // Window
    m_Window.Init();

    // Vulkan core
    m_Instance.Create();
    m_Instance.SetupDebugMessenger();
    m_Window.CreateSurface(m_Instance.Get());
    m_Device.Init(&m_Instance, m_Window.GetSurface());

    // Swap chain
    m_SwapChain.Create(&m_Device, m_Window.GetSurface(), m_Window.GetHandle());
    m_SwapChain.CreateImageViews();

    // Commands (needed before resource uploads)
    m_CommandManager.Init(&m_Device);
    m_CommandManager.AllocateCommandBuffers();

    // Render pass
    m_RenderPass.Create(&m_Device, m_SwapChain.GetFormat());

    // Resources
    std::string modelPath   = std::string(PROJECT_SOURCE_DIR) + "/Models/viking_room.obj";
    std::string texturePath = std::string(PROJECT_SOURCE_DIR) + "/Textures/viking_room.png";

    Mesh mesh = ModelLoader::Load(modelPath);

    m_Buffer.UploadMesh(&m_Device, &m_CommandManager, mesh);
    m_Texture.Load(&m_Device, &m_CommandManager,texturePath);

    // Uniform buffers
    m_UniformBuffer.Create(&m_Device, CommandManager::MAX_FRAMES_IN_FLIGHT);

    // Descriptors
    m_Descriptors.CreateLayout(&m_Device);
    m_Descriptors.CreatePool(&m_Device, CommandManager::MAX_FRAMES_IN_FLIGHT);

    std::vector<VkBuffer> uboBuffers(CommandManager::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < CommandManager::MAX_FRAMES_IN_FLIGHT; i++)
        uboBuffers[i] = m_UniformBuffer.GetBuffer(i);

    m_Descriptors.CreateSets(&m_Device, CommandManager::MAX_FRAMES_IN_FLIGHT,
        uboBuffers, m_Texture.GetView(), m_Texture.GetSampler());

    // Pipeline
    m_Pipeline.Create(&m_Device, m_RenderPass.Get(), m_Descriptors.GetLayout());

    // Swap-chain dependents
    CreateSwapChainDependents();

    // Sync objects
    CreateSyncObjects();

    // Camera
    float aspect = m_SwapChain.GetExtent().width / (float)m_SwapChain.GetExtent().height;
    m_Camera.SetPerspective(45.0f, aspect, 0.1f, 100.0f);
    m_Camera.LookAt({ 2.0f, 2.0f, 2.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f });
}

void Renderer::CreateSwapChainDependents()
{
    m_DepthBuffer.Create(&m_Device, m_SwapChain.GetExtent());
    m_FrameBuffer.Create(&m_Device, m_RenderPass.Get(),
        m_SwapChain.GetImageViews(),
        m_DepthBuffer.GetView(),
        m_SwapChain.GetExtent());
}

void Renderer::DestroySwapChainDependents()
{
    m_DepthBuffer.Destroy();
    m_FrameBuffer.Destroy();
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
}

void Renderer::MainLoop()
{
    while (!m_Window.ShouldClose())
    {
        m_Window.PollEvents();
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

    VkSemaphore waitSemaphores[]   = { m_ImageAvailable[m_CurrentFrame] };
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

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = m_RenderPass.Get();
    rpInfo.framebuffer = m_FrameBuffer.Get()[imageIndex];
    rpInfo.renderArea.offset = { 0, 0 };
    rpInfo.renderArea.extent = m_SwapChain.GetExtent();
    rpInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    rpInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline.Get());

    VkViewport viewport{};
    viewport.x = 0.0f; viewport.y = 0.0f;
    viewport.width  = static_cast<float>(m_SwapChain.GetExtent().width);
    viewport.height = static_cast<float>(m_SwapChain.GetExtent().height);
    viewport.minDepth = 0.0f; viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{ {0, 0}, m_SwapChain.GetExtent() };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    VkBuffer vertexBuffers[] = { m_Buffer.GetVertexBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmd, m_Buffer.GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

    VkDescriptorSet set = m_Descriptors.GetSet(m_CurrentFrame);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_Pipeline.GetLayout(), 0, 1, &set, 0, nullptr);

    vkCmdDrawIndexed(cmd, m_Buffer.GetIndexCount(), 1, 0, 0, 0);
    vkCmdEndRenderPass(cmd);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
        throw std::runtime_error("Failed to record command buffer!");
}

void Renderer::UpdateUniformBuffer(uint32_t frame)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view  = m_Camera.GetView();
    ubo.proj  = m_Camera.GetProjection();

    m_UniformBuffer.Update(frame, ubo);
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

    m_Texture.Destroy();
    m_Buffer.Destroy();
    m_UniformBuffer.Destroy(CommandManager::MAX_FRAMES_IN_FLIGHT);
    m_Descriptors.DestroyPool();
    m_Descriptors.DestroyLayout();
    m_Pipeline.Destroy();
    m_RenderPass.Destroy();

    DestroySyncObjects();
    m_CommandManager.Destroy();
    m_Device.Destroy();
    m_Instance.DestroyDebugMessenger();
    m_Window.DestroySurface(m_Instance.Get());
    m_Instance.Destroy();
    m_Window.Destroy();
}
