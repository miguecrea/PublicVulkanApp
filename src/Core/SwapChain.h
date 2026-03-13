#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Device;

class SwapChain
{
public:
    SwapChain() = default;
    ~SwapChain() = default;

    void Create(Device* device, VkSurfaceKHR surface, GLFWwindow* window);
    void CreateImageViews();
    void Destroy();
    void DestroyImageViews();

    VkSwapchainKHR Get() const { return m_SwapChain; }
    VkFormat GetFormat() const { return m_Format; }
    VkExtent2D GetExtent() const { return m_Extent; }
    const std::vector<VkImageView>& GetImageViews() const { return m_ImageViews; }
    uint32_t GetImageCount() const { return static_cast<uint32_t>(m_Images.size()); }

private:
    Device* m_Device = nullptr;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

    VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;
    VkFormat m_Format;
    VkExtent2D m_Extent;

    std::vector<VkImage> m_Images;
    std::vector<VkImageView> m_ImageViews;

    VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& modes);
    VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
};
