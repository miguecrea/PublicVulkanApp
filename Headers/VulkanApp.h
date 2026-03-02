#pragma once

#pragma once
#include "../Headers/VulkanApp.h"

class Window;
class InstanceManager;
class DeviceManager;


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
    InstanceManager * m_InstanceManager;
    DeviceManager * m_DeviceManager;
    void  setupDebugMessenger();
};


