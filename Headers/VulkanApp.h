#pragma once

#pragma once
#include "../Headers/VulkanApp.h"

class Window;
class Instance;

class VulkanApp final
{
public:
    VulkanApp();
    ~VulkanApp();
    void Run();
private:

    void InitWindow();
    void InitVulkan();
    void MainLoop();
    void CleanUp();

    Window * m_Window;
    Instance * m_Instance;


   void  setupDebugMessenger();

    //Validation Layers 
 //   VkDebugUtilsMessengerEXT debugMessenger;

};


