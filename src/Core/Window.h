#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cstdint>

class Window
{
public:
    Window(uint32_t width = 800, uint32_t height = 600);
    ~Window() = default;

    void Init();
    void Destroy();

    GLFWwindow* GetHandle() const { return m_Window; }
    VkSurfaceKHR GetSurface() const { return m_Surface; }

    void CreateSurface(VkInstance instance);
    void DestroySurface(VkInstance instance);

    bool ShouldClose() const { return glfwWindowShouldClose(m_Window); }
    void PollEvents() const { glfwPollEvents(); }
    void GetFramebufferSize(int& width, int& height) const;
    void WaitEvents() const { glfwWaitEvents(); }

    bool FramebufferResized = false;

private:
    GLFWwindow* m_Window = nullptr;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    uint32_t m_Width;
    uint32_t m_Height;

    static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
};
