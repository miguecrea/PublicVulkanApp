#include "SwapChain.h"
#include "Device.h"
#include <stdexcept>
#include <algorithm>
#include <limits>

void SwapChain::Create(Device* device, VkSurfaceKHR surface, GLFWwindow* window)
{
    m_Device = device;
    m_Surface = surface;

    auto support = Device::QuerySwapChainSupport(device->GetPhysical(), surface);
    auto surfaceFormat = ChooseSurfaceFormat(support.formats);
    auto presentMode = ChoosePresentMode(support.presentModes);
    auto extent = ChooseExtent(support.capabilities, window);

    uint32_t imageCount = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount)
        imageCount = support.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const auto& indices = device->GetQueueFamilies();
    uint32_t queueFamilyIndices[] = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value()
    };

    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = support.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device->GetLogical(), &createInfo, nullptr, &m_SwapChain) != VK_SUCCESS)
        throw std::runtime_error("Failed to create swap chain!");

    vkGetSwapchainImagesKHR(device->GetLogical(), m_SwapChain, &imageCount, nullptr);
    m_Images.resize(imageCount);
    vkGetSwapchainImagesKHR(device->GetLogical(), m_SwapChain, &imageCount, m_Images.data());

    m_Format = surfaceFormat.format;
    m_Extent = extent;
}

void SwapChain::CreateImageViews()
{
    m_ImageViews.resize(m_Images.size());
    for (size_t i = 0; i < m_Images.size(); i++)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_Images[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = m_Format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_Device->GetLogical(), &viewInfo, nullptr, &m_ImageViews[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create swap chain image view!");
    }
}

void SwapChain::Destroy()
{
    vkDestroySwapchainKHR(m_Device->GetLogical(), m_SwapChain, nullptr);
}

void SwapChain::DestroyImageViews()
{
    for (auto view : m_ImageViews)
        vkDestroyImageView(m_Device->GetLogical(), view, nullptr);
    m_ImageViews.clear();
}

VkSurfaceFormatKHR SwapChain::ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
    // The Narkowicz ACES approximation already produces display-encoded (gamma-compressed)
    // values. Using an _SRGB swapchain would apply γ=2.2 a second time and wash out
    // all PBR contrast. Prefer UNORM so the ACES output is stored as-is.
    for (const auto& f : formats)
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return f;
    return formats[0];
}

VkPresentModeKHR SwapChain::ChoosePresentMode(const std::vector<VkPresentModeKHR>& modes)
{
    for (const auto& m : modes)
        if (m == VK_PRESENT_MODE_MAILBOX_KHR)
            return m;
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChain::ChooseExtent(const VkSurfaceCapabilitiesKHR & capabilities, GLFWwindow* window)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    VkExtent2D extent = { static_cast<uint32_t>(w), static_cast<uint32_t>(h) };
    extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    return extent;
}
