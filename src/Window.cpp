#include "../Headers/Window.h"

Window::Window()
{
}

GLFWwindow * Window::GetWindow()
{
    return m_WindowPointer;
}

void Window::initWindow()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_WindowPointer = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

