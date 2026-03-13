#include "Window.h"
#include <stdexcept>

Window::Window(uint32_t width, uint32_t height)
    : m_Width(width), m_Height(height)
{
}

void Window::Init()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_Window = glfwCreateWindow(m_Width, m_Height, "Vulkan Renderer", nullptr, nullptr);
    glfwSetWindowUserPointer(m_Window, this);
    glfwSetFramebufferSizeCallback(m_Window, FramebufferResizeCallback);
}

void Window::Destroy()
{
    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

void Window::CreateSurface(VkInstance instance)
{
    if (glfwCreateWindowSurface(instance, m_Window, nullptr, &m_Surface) != VK_SUCCESS)
        throw std::runtime_error("Failed to create window surface!");
}

void Window::DestroySurface(VkInstance instance)
{
    vkDestroySurfaceKHR(instance, m_Surface, nullptr);
}

void Window::GetFramebufferSize(int& width, int& height) const
{
    glfwGetFramebufferSize(m_Window, &width, &height);
}

void Window::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto* w = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    w->FramebufferResized = true;
}
