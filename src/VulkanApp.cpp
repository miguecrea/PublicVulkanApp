#include"../Headers/Core/VulkanApp.h"
#include"../Headers/Core/Window.h"
#include"../Headers/Core/InstanceManager.h"
#include"../Headers/Core/DeviceManager.h"
#include"../Headers/Core/SwapChain.h"
#include"../Headers/Core/GraphicsPipeline.h"
#include"../Headers/Core/RenderPass.h"
#include"../Headers/Core/FrameBuffer.h"
#include"../Headers/Core/CommandManager.h"
#include"../Headers/Core/VertexBuffer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include"iostream"

Renderer::Renderer()
{
	m_Window = new Window();
	m_InstanceManager = new InstanceManager();
	m_DeviceManager = new DeviceManager();
	m_SwapChain = new SwapChain(m_DeviceManager);
	m_GraphicsPipeline = new GraphicsPipeline();
}
Renderer::~Renderer()
{
	delete m_Window;
	m_Window = nullptr;

	delete m_InstanceManager;
	m_InstanceManager = nullptr;

	delete m_DeviceManager;
	m_DeviceManager = nullptr;

	delete m_SwapChain;
	m_SwapChain = nullptr;

	delete m_GraphicsPipeline;
	m_GraphicsPipeline = nullptr;

	delete m_RenderPass;
	m_RenderPass = nullptr;

	delete m_FrameBuffer;
	m_FrameBuffer = nullptr;

	delete m_CommandManager;
	m_CommandManager = nullptr;

	delete m_vertexBuffer;
	m_vertexBuffer = nullptr;


}
void Renderer::InitWindow()
{
	if (m_Window)
	{
		m_Window->initWindow();
	}
	else
	{
		std::cout << "Window Pointer is not valid\n";
	}
}

void Renderer::InitVulkan()
{
	m_InstanceManager->CreateInstance();
	m_InstanceManager->setupDebugMessenger();
	m_Window->createSurface(m_InstanceManager->GetVulkanInstance());
	//SET SURFACE TO BE USED FINDING QUEUUE FAMILY
	m_DeviceManager->SetSurface(m_Window->surface);
	m_DeviceManager->pickPhysicalDevice(m_InstanceManager->GetVulkanInstance());
	m_DeviceManager->createLogicalDevice(m_InstanceManager);
	m_SwapChain->createSwapChain(m_DeviceManager->GetPhysicalDevice(),m_DeviceManager->GetLogicalDevice(), m_Window->surface, m_Window->GetWindow());
	m_SwapChain->createImageViews(m_DeviceManager->GetLogicalDevice());
	m_RenderPass = new RenderPass(m_DeviceManager->GetLogicalDevice(), m_SwapChain->GetSwapChainImageFormat());
	m_RenderPass->CreateRenderPass();
	m_GraphicsPipeline->CreateGraphicsPipeline(m_DeviceManager->GetLogicalDevice(),m_RenderPass->Get());
	m_FrameBuffer = new FramebufferManager(m_DeviceManager->GetLogicalDevice());
	m_FrameBuffer->CreateFramebuffers(m_RenderPass->Get(), m_SwapChain->GetSwapChainImageViews(), m_SwapChain->GetExtend());
	m_CommandManager = new CommandManager(m_DeviceManager->GetLogicalDevice(), m_DeviceManager->GetFamilyIndices());
	m_vertexBuffer = new BufferManager(m_DeviceManager);
	m_CommandManager->CreateCommandPool();
	m_vertexBuffer->CreateVertexBuffer(m_CommandManager->GetCommandPool());
	m_CommandManager->createCommandBuffer();
	createSemaphoresObjects(m_DeviceManager->GetLogicalDevice()); //NO CLASSS 
}

void Renderer::MainLoop()
{
	while (!glfwWindowShouldClose(m_Window->GetWindow()))
	{
		glfwPollEvents();
		DrawFrame();
	}
	vkDeviceWaitIdle(m_DeviceManager->GetLogicalDevice());
}

void Renderer::Run()
{
	InitWindow();
	InitVulkan();
	MainLoop();
	CleanUp();
}
void Renderer::DrawFrame()
{
#pragma region Explanation



	
	/*
	    Wait for the previous frame to finish
		Acquire an image from the swap chain
		Record a command buffer which draws the scene onto that image
		Submit the recorded command buffer
		Present the swap chain image*/

	// Each of these events is set in motion using a single function call, but are all executed asynchronously. The function calls will return before the operations are actually 
	// finished and the order of execution is also undefined.


	// Semaphores
	//A semaphore is used to add order between queue operations.

	
	//VkCommandBuffer A, B = ... // record command buffers
	//	VkSemaphore S = ... // create a semaphore

	//	// enqueue A, signal S when done - starts executing immediately
	//	vkQueueSubmit(work: A, signal : S, wait : None)

	//	// enqueue B, wait on S to start
	//	vkQueueSubmit(work: B, signal : None, wait : S)


	//SEMAPHORES ARE ONLY CPU 
	// we need to stop GPU AS well 
	// Note that in this code snippet, both calls to vkQueueSubmit() return immediately - the waiting only happens on the GPU.
	// The CPU continues running without blocking. To make the CPU wait, we need a different synchronization primitive,
	// which we will now describe.


	//Similar to semaphores, fences are either in a signaled or unsignaled state.
	// Whenever we submit work to execute, we can attach a fence to that work.When the work is finished,
	// the fence will be signaled.Then we can make the host wait for the fence to be signaled, 
	// guaranteeing that the work has finished before the host continues.

	// VkCommandBuffer A = ... // record command buffer with the transfer
	    //VkFence F = ... // create the fence

		// enqueue A, start work immediately, signal F when done
		//vkQueueSubmit(work: A, fence : F) 

		//vkWaitForFence(F) // blocks execution until A has finished executing

		//save_screenshot_to_disk() 
#pragma endregion
	// either any or all the fences , and timeout parameter
	vkWaitForFences(m_DeviceManager->GetLogicalDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
	//reset to unsiglaned state 

	//get image from swap Chain
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(m_DeviceManager->GetLogicalDevice(),m_SwapChain->Get(), UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		RecreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	vkResetFences(m_DeviceManager->GetLogicalDevice(), 1, &inFlightFences[currentFrame]);

	//reset before recording 
	vkResetCommandBuffer(m_CommandManager->GetCommandBuffersVector()[currentFrame], 0);

	//record CommandBuffer
	m_CommandManager->recordCommandBuffer(m_CommandManager->GetCommandBuffersVector()[currentFrame],
		imageIndex, m_RenderPass->Get(), m_FrameBuffer->GetFrameBuffers(), m_GraphicsPipeline->GetPipeline(),
		m_SwapChain->GetExtend(),m_vertexBuffer);


	//submit command buffer 
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;


	// he first three parameters specify which semaphores to wait on before execution begins and in which stage(s) of
	// the pipeline to wait
	VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;


	//The next two parameters specify which command buffers to actually submit for execution.We simply submit the single command buffer we have.
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_CommandManager->GetCommandBuffersVector()[currentFrame];


	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;


	if (vkQueueSubmit(m_DeviceManager->GetGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	// The last step of drawing a frame is submitting the result back to the swap chain 
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;  //we want to wait on this semaphore

	VkSwapchainKHR swapChains[] = {m_SwapChain->Get()};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(m_DeviceManager->GetPresentQueue(), &presentInfo);


	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
		framebufferResized = false;
		RecreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	currentFrame = (currentFrame + 1) % m_CommandManager->MAX_FRAMES_IN_FLIGHT;
}
void Renderer::createSemaphoresObjects(VkDevice device)
{
	imageAvailableSemaphores.resize(m_CommandManager->MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(m_CommandManager->MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(m_CommandManager->MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;  //need to make it signaled 

	for (size_t i = 0; i < m_CommandManager->MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}
}

void Renderer::DestroySemaphoresObjects(VkDevice device)
{

	for (size_t i = 0; i < m_CommandManager->MAX_FRAMES_IN_FLIGHT; i++) 
	{
		vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(device, inFlightFences[i], nullptr);
	}

}

void Renderer::RecreateSwapChain()
{

	int width = 0, height = 0;
	glfwGetFramebufferSize(m_Window->GetWindow(), &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(m_Window->GetWindow(), &width, &height);
		glfwWaitEvents();
	}



	vkDeviceWaitIdle(m_DeviceManager->GetLogicalDevice());

	CleanUpSwapChain();

	m_SwapChain->createSwapChain(m_DeviceManager->GetPhysicalDevice(), m_DeviceManager->GetLogicalDevice(), m_Window->surface, m_Window->GetWindow());
	m_SwapChain->createImageViews(m_DeviceManager->GetLogicalDevice());
	m_FrameBuffer->CreateFramebuffers(m_RenderPass->Get(), m_SwapChain->GetSwapChainImageViews(), m_SwapChain->GetExtend());

}

void Renderer::CleanUpSwapChain()
{
	m_FrameBuffer->DestroyFrameBuffers();
	m_SwapChain->DestroyImageViews(m_DeviceManager->GetLogicalDevice());
	m_SwapChain->DestroySwapChain(m_DeviceManager->GetLogicalDevice());

}

void Renderer::CleanUp()
{

	CleanUpSwapChain();

	m_vertexBuffer->DestroyBuffer();
	m_vertexBuffer->FreeMemoryBuffer();

	DestroySemaphoresObjects(m_DeviceManager->GetLogicalDevice());
	m_CommandManager->DestroyCommandPool();
	m_GraphicsPipeline->DestroyPipeline(m_DeviceManager->GetLogicalDevice());
	m_GraphicsPipeline->DestroyPipelineLayout(m_DeviceManager->GetLogicalDevice());
	m_RenderPass->DestroyRenderPass();
	m_DeviceManager->DestroyLogicalDevice();
	m_InstanceManager->DestroyValidationLayers();
	m_Window->DestroySurface(m_InstanceManager->GetVulkanInstance());
	vkDestroyInstance(m_InstanceManager->GetVulkanInstance(), nullptr);
	glfwDestroyWindow(m_Window->GetWindow());
	glfwTerminate();
}
