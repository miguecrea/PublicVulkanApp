#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Window
{
private:
    GLFWwindow * m_WindowPointer;
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;
public:
    Window();
    void initWindow();
    GLFWwindow * GetWindow();
    void createSurface(VkInstance Vulkaninstance);
    void DestroySurface(VkInstance Vulkaninstance);
    VkSurfaceKHR surface;

};
