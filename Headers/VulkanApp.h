#pragma once

#pragma once
#include "../Headers/VulkanApp.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
class VulkanApp
{
public:
    VulkanApp();
    void InitWindow();
    void Run();
private:


};


