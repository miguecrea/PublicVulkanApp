#include"../Headers/VulkanApp.h"
#include"../Headers/Window.h"
#include"../Headers/InstanceManager.h"
#include"../Headers/DeviceManager.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include"iostream"

VulkanApp::VulkanApp()
{
	m_Window = new Window();
	m_InstanceManager = new InstanceManager();
	m_DeviceManager = new DeviceManager();
}
VulkanApp::~VulkanApp()
{
	delete m_Window;
	m_Window = nullptr;

	delete m_InstanceManager;
	m_InstanceManager = nullptr;

	delete m_DeviceManager;
	m_DeviceManager = nullptr;

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
	m_DeviceManager->DestroyLogicalDevice();
	m_InstanceManager->DestroyValidationLayers();
	m_Window->DestroySurface(m_InstanceManager->GetVulkanInstance());
	vkDestroyInstance(m_InstanceManager->GetVulkanInstance(), nullptr);
	glfwDestroyWindow(m_Window->GetWindow());
	glfwTerminate();
}
