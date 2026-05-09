#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>

class Device;

struct UniformBufferObject
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

struct MaterialUBO
{

    alignas(16) glm::vec4 baseColorFactor;
    alignas(16) glm::vec4 emissiveFactor;       // xyz = factor, w unused
    alignas(4)  float metallicFactor;
    alignas(4)  float roughnessFactor;
    alignas(4)  float hasNormalMap;
    alignas(4)  float hasMetallicRoughness;
    alignas(4)  float hasAO;
    alignas(4)  float hasEmissive;
    alignas(4)  float alphaMask;
    alignas(4)  float alphaCutoff;
};


constexpr int MAX_SPOT_LIGHTS  = 8;
constexpr int MAX_POINT_LIGHTS = 8;

struct SpotLight
{
    alignas(16) glm::vec4 position;   // xyz = world pos,  w = range (meters)
    alignas(16) glm::vec4 direction;  // xyz = direction,  w = cos(innerCone)
    alignas(16) glm::vec4 color;      // xyz = RGB color,  w = intensity (lumens)
    float outerCone;                  // cos(outerCone)
    float _pad[3];
};

struct PointLight
{
    alignas(16) glm::vec4 position;   // xyz = world pos, w = range (meters)
    alignas(16) glm::vec4 color;      // xyz = RGB color, w = intensity (lumens)
};

struct LightUBO
{
    alignas(16) glm::mat4 lightSpaceMatrix;                // directional light VP matrix
    alignas(16) glm::vec4 dirLightDir;
    alignas(16) glm::vec4 dirLightColor;   // xyz = color, w = illuminance (lux)
    alignas(16) glm::vec4 camPos;
    alignas(16) glm::vec4 skyLight;        // xyz = sky color, w = sky irradiance (lux)
    alignas(4)  float aperture;            // f-stops
    alignas(4)  float shutterSpeed;        // seconds
    alignas(4)  float iso;
    alignas(4)  int   spotLightCount;
    alignas(4)  int   pointLightCount;
    alignas(4)  int   useIBL;              // 1 = sample irradiance cubemap, 0 = hemisphere
    alignas(4)  float iblIntensity;        // IBL scale factor
    alignas(4)  float _padLight;
    SpotLight  spotLights[MAX_SPOT_LIGHTS];
    PointLight pointLights[MAX_POINT_LIGHTS];
};

class UniformBuffer
{
public:
    UniformBuffer() = default;
    ~UniformBuffer() = default;

    void Create(Device* device, int framesInFlight);
    void Destroy(int framesInFlight);

    void Update(uint32_t frame, const UniformBufferObject& ubo);
    void UpdateLight(uint32_t frame, const LightUBO& light);
    void UpdateMaterial(int materialIndex, const MaterialUBO& mat);

    VkBuffer GetBuffer(int frame)         const { return m_CameraBuffers[frame]; }
    VkBuffer GetLightBuffer(int frame)    const { return m_LightBuffers[frame]; }
    VkBuffer GetMaterialBuffer(int index) const { return m_MaterialBuffers[index]; }

    int GetMaterialCount() const { return (int)m_MaterialBuffers.size(); }

    void CreateMaterialBuffers(Device* device, int count);
    void DestroyMaterialBuffers();

private:
    Device* m_Device = nullptr;

    // Per-frame camera UBOs
    std::vector<VkBuffer>       m_CameraBuffers;
    std::vector<VkDeviceMemory> m_CameraMemories;
    std::vector<void*>          m_CameraMapped;

    // Per-frame light UBOs
    std::vector<VkBuffer>       m_LightBuffers;
    std::vector<VkDeviceMemory> m_LightMemories;
    std::vector<void*>          m_LightMapped;

    // Per-material UBOs (not per-frame, updated once on load)
    std::vector<VkBuffer>       m_MaterialBuffers;
    std::vector<VkDeviceMemory> m_MaterialMemories;
    std::vector<void*>          m_MaterialMapped;

    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
        VkMemoryPropertyFlags props, VkBuffer& buf, VkDeviceMemory& mem);
    uint32_t FindMemoryType(uint32_t filter, VkMemoryPropertyFlags props);
};