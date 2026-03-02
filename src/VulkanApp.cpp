#include"../Headers/VulkanApp.h"
#include"../Headers/Window.h"
#include"../Headers/Instance.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include"iostream"

VulkanApp::VulkanApp()
{
	m_Window = new Window();
	m_Instance = new Instance();
}
VulkanApp::~VulkanApp()
{
	delete m_Window;
	m_Window = nullptr;

	delete m_Instance;
	m_Instance = nullptr;

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
	m_Instance->CreateInstance();
	
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

	/*if (enableValidationLayers) 
	{
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}*/
	vkDestroyInstance(m_Instance->GetInstance(), nullptr);
	glfwDestroyWindow(m_Window->GetWindow());
	glfwTerminate();
}
