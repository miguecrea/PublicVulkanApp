#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <array>

class Device;

class Descriptors
{
public:
    Descriptors() = default;
    ~Descriptors() = default;

    // --- Set 0: per-frame camera UBO ---
    void CreateCameraLayout(Device* device);
    void CreateCameraPool(Device* device, int framesInFlight);
    void CreateCameraSets(Device* device, int framesInFlight,
        const std::vector<VkBuffer>& cameraBuffers);
    void DestroyCameraLayout();
    void DestroyCameraPool();

    VkDescriptorSetLayout GetCameraLayout()      const { return m_CameraLayout; }
    VkDescriptorSet       GetCameraSet(int frame) const { return m_CameraSets[frame]; }

    // --- Set 1: per-material (albedo, normal, material UBO) ---
    void CreateMaterialLayout(Device* device);
    void CreateMaterialPool(Device* device, int materialCount);
    void CreateMaterialSets(Device* device, int materialCount,
        const std::vector<VkBuffer>& materialUBOs,
        const std::vector<VkImageView>& albedoViews,
        const std::vector<VkSampler>& albedoSamplers,
        const std::vector<VkImageView>& normalViews,
        const std::vector<VkSampler>& normalSamplers,
        VkImageView fallbackView, VkSampler fallbackSampler);
    void DestroyMaterialLayout();
    void DestroyMaterialPool();

    VkDescriptorSetLayout GetMaterialLayout()         const { return m_MaterialLayout; }
    VkDescriptorSet       GetMaterialSet(int material) const { return m_MaterialSets[material]; }

    // --- Lighting pass (4 input attachments + light UBO) ---
    void CreateLightingLayout(Device* device);
    void CreateLightingPool(Device* device);
    void CreateLightingSet(Device* device,
        const std::array<VkImageView, 4>& gbufferViews,
        const std::vector<VkBuffer>& lightBuffers, int framesInFlight);
    void DestroyLightingLayout();
    void DestroyLightingPool();

    VkDescriptorSetLayout GetLightingLayout()         const { return m_LightingLayout; }
    VkDescriptorSet       GetLightingSet(int frame)    const { return m_LightingSets[frame]; }

    // Tone mapping (reads HDR image)
    void CreateToneMappingLayout(Device* device);
    void CreateToneMappingPool(Device* device);
    void CreateToneMappingSet(Device* device, VkImageView hdrView,
        const std::vector<VkBuffer>& lightBuffers, int framesInFlight);
    void DestroyToneMappingLayout();
    void DestroyToneMappingPool();

    VkDescriptorSetLayout GetToneMappingLayout()         const { return m_ToneMappingLayout; }
    VkDescriptorSet       GetToneMappingSet(int frame)    const { return m_ToneMappingSets[frame]; }

private:


    // TONE mapping 
    VkDescriptorSetLayout        m_ToneMappingLayout = VK_NULL_HANDLE;
    VkDescriptorPool             m_ToneMappingPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> m_ToneMappingSets;


    Device* m_Device = nullptr;

    // Camera
    VkDescriptorSetLayout        m_CameraLayout = VK_NULL_HANDLE;
    VkDescriptorPool             m_CameraPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> m_CameraSets;

    // Material
    VkDescriptorSetLayout        m_MaterialLayout = VK_NULL_HANDLE;
    VkDescriptorPool             m_MaterialPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> m_MaterialSets;

    // Lighting
    VkDescriptorSetLayout        m_LightingLayout = VK_NULL_HANDLE;
    VkDescriptorPool             m_LightingPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> m_LightingSets;
};