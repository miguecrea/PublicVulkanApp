#include "../Headers/Window.h"
#include <vector>
#include <stdexcept>
#include <iostream>


Window::Window()
{
}

void Window::initWindow()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_WindowPointer = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
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


