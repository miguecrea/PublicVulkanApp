#include "../Headers/Core/Window.h"
#include <vector>
#include <stdexcept>
#include <iostream>
#include"../Headers/Core/FrameBuffer.h"
#include"../Headers/Core/VulkanApp.h"


static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

Window::Window()
{
}

void Window::initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_WindowPointer = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(m_WindowPointer, this);
    glfwSetFramebufferSizeCallback(m_WindowPointer,framebufferResizeCallback);
}

GLFWwindow * Window::GetWindow()
{
    return m_WindowPointer;
}

void Window::createSurface(VkInstance Vulkaninstance)
{
    if (glfwCreateWindowSurface(Vulkaninstance,m_WindowPointer, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface!");
    }
}
void Window::DestroySurface(VkInstance Vulkaninstance)
{
    vkDestroySurfaceKHR(Vulkaninstance, surface, nullptr);
}

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto app = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}


