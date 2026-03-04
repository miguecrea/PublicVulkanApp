#include"../Headers/VulkanApp.h"
#include"../Headers/Window.h"
#include"../Headers/InstanceManager.h"
#include"../Headers/DeviceManager.h"
#include"../Headers/SwapChain.h"
#include"../Headers/GraphicsPipeline.h"
#include"../Headers/RenderPass.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include"iostream"

VulkanApp::VulkanApp()
{
	m_Window = new Window();
	m_InstanceManager = new InstanceManager();
	m_DeviceManager = new DeviceManager();
	m_SwapChain = new SwapChain(m_DeviceManager);
	m_GraphicsPipeline = new GraphicsPipeline();
}
VulkanApp::~VulkanApp()
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

}
void VulkanApp::InitWindow()
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

void VulkanApp::InitVulkan()
{
	m_InstanceManager->CreateInstance();
	m_InstanceManager->setupDebugMessenger();
	m_Window->createSurface(m_InstanceManager->GetVulkanInstance());

	//SET SURFACE TO BE USED FINDING QUEUUE FAMILY
	m_DeviceManager->SetSurface(m_Window->surface);

	m_DeviceManager->pickPhysicalDevice(m_InstanceManager->GetVulkanInstance());
	m_DeviceManager->createLogicalDevice(m_InstanceManager);
	m_SwapChain->createSwapChain(m_DeviceManager->GetPhysicalDevice(),m_DeviceManager->GetLogicalDevice(), m_Window->surface, m_Window->GetWindow());
	//An image view is sufficient to start using an image as a texture, but it's not quite ready to be used as a render target just yet. 
	// That requires one more step of indirection, known as a framebuffer.
	//But first we'll have to set up the graphics pipeline.
	m_SwapChain->createImageViews(m_DeviceManager->GetLogicalDevice());



	m_RenderPass = new RenderPass(m_DeviceManager->GetLogicalDevice(), m_SwapChain->GetSwapChainImageFormat());
	m_RenderPass->CreateRenderPass();


	m_GraphicsPipeline->CreateGraphicsPipeline(m_DeviceManager->GetLogicalDevice(),m_RenderPass->Get());

}

void VulkanApp::MainLoop()
{
	while (!glfwWindowShouldClose(m_Window->GetWindow()))
	{
		glfwPollEvents();
	}
}

void VulkanApp::Run()
{
	InitWindow();
	InitVulkan();
	MainLoop();
	CleanUp();
}
void VulkanApp::CleanUp()
{
	m_GraphicsPipeline->DestroyPipeline(m_DeviceManager->GetLogicalDevice());
	m_GraphicsPipeline->DestroyPipelineLayout(m_DeviceManager->GetLogicalDevice());
	m_RenderPass->DestroyRenderPass();
	m_SwapChain->DestroyImageViews(m_DeviceManager->GetLogicalDevice());
	m_SwapChain->DestroySwapChain(m_DeviceManager->GetLogicalDevice());
	m_DeviceManager->DestroyLogicalDevice();
	m_InstanceManager->DestroyValidationLayers();
	m_Window->DestroySurface(m_InstanceManager->GetVulkanInstance());
	vkDestroyInstance(m_InstanceManager->GetVulkanInstance(), nullptr);
	glfwDestroyWindow(m_Window->GetWindow());
	glfwTerminate();
}
