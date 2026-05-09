#pragma once
#include <vulkan/vulkan.h>
#include <string>

class Device;
class CommandManager;

// Image-Based Lighting helper.
// Loads an HDR equirectangular map, converts it to a cubemap, then produces:
//   - Diffuse irradiance cubemap (32×32, for diffuse ambient)
//   - Prefiltered specular cubemap (128×128, 5 mip levels by roughness)
//   - BRDF integration LUT (256×256, split-sum approximation)
// All steps run once at startup via compute shaders.
//
// When the HDR file is absent, fallback 1×1 resources are created so
// descriptors are always valid. IsValid() returns false in that case
// and the shader uses the hemisphere ambient fallback instead.
class IBL
{
public:
    static constexpr uint32_t ENV_SIZE             = 512;
    static constexpr uint32_t IRR_SIZE             = 32;
    static constexpr uint32_t PREFILTER_SIZE       = 128;
    static constexpr uint32_t PREFILTER_MIP_LEVELS = 5;
    static constexpr uint32_t BRDF_LUT_SIZE        = 256;

    IBL()  = default;
    ~IBL() = default;

    bool Create(Device* device, CommandManager* cmdManager, const std::string& hdrPath);
    void Destroy();

    VkImageView GetIrradianceView()    const { return m_IrradianceView;    }
    VkSampler   GetIrradianceSampler() const { return m_IrradianceSampler; }
    VkImageView GetPrefilterView()     const { return m_PrefilterView;     }
    VkSampler   GetPrefilterSampler()  const { return m_PrefilterSampler;  }
    VkImageView GetBrdfLutView()       const { return m_BrdfLUTView;       }
    VkSampler   GetBrdfLutSampler()    const { return m_BrdfLUTSampler;    }
    bool        IsValid()              const { return m_Valid;              }

private:
    Device* m_Device = nullptr;
    bool    m_Valid  = false;

    // Intermediate environment cubemap (destroyed after precomputation)
    VkImage        m_EnvCube    = VK_NULL_HANDLE;
    VkDeviceMemory m_EnvMemory  = VK_NULL_HANDLE;
    VkImageView    m_EnvView    = VK_NULL_HANDLE;
    VkSampler      m_EnvSampler = VK_NULL_HANDLE;

    // Diffuse irradiance cubemap
    VkImage        m_Irradiance       = VK_NULL_HANDLE;
    VkDeviceMemory m_IrrMemory        = VK_NULL_HANDLE;
    VkImageView    m_IrradianceView   = VK_NULL_HANDLE;
    VkSampler      m_IrradianceSampler = VK_NULL_HANDLE;

    // Prefiltered specular environment cubemap (multi-mip)
    VkImage        m_Prefilter        = VK_NULL_HANDLE;
    VkDeviceMemory m_PrefilterMemory  = VK_NULL_HANDLE;
    VkImageView    m_PrefilterView    = VK_NULL_HANDLE;
    VkSampler      m_PrefilterSampler = VK_NULL_HANDLE;

    // BRDF integration look-up table
    VkImage        m_BrdfLUT        = VK_NULL_HANDLE;
    VkDeviceMemory m_BrdfLUTMemory  = VK_NULL_HANDLE;
    VkImageView    m_BrdfLUTView    = VK_NULL_HANDLE;
    VkSampler      m_BrdfLUTSampler = VK_NULL_HANDLE;

    // ---- helpers ----
    void CreateFallback(CommandManager* cmdManager);

    void CreateCubemapImage(uint32_t size, VkFormat format, VkImageUsageFlags usage,
        VkImage& outImage, VkDeviceMemory& outMemory, uint32_t mipLevels = 1);

    void CreateImage2D(uint32_t width, uint32_t height, VkFormat format,
        VkImageUsageFlags usage, VkImage& outImage, VkDeviceMemory& outMemory);

    VkImageView CreateCubeView(VkImage image, VkFormat format, uint32_t mipLevels = 1);
    VkImageView CreateFaceView(VkImage image, VkFormat format, uint32_t face, uint32_t mip = 0);
    VkImageView Create2DView(VkImage image, VkFormat format);

    void BarrierCubemap(VkCommandBuffer cmd, VkImage image,
        VkImageLayout oldLayout, VkImageLayout newLayout,
        VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
        VkAccessFlags srcAccess, VkAccessFlags dstAccess,
        uint32_t mipLevels = 1);

    void RunPrecompute(CommandManager* cmdManager,
        VkImage equirectImage, VkImageView equirectView, VkSampler equirectSampler);

    void RunSpecularPrecompute(CommandManager* cmdManager);
};
