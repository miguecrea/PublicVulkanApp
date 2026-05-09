#version 450

layout(input_attachment_index = 0, binding = 0) uniform subpassInput inPosition;
layout(input_attachment_index = 1, binding = 1) uniform subpassInput inNormal;
layout(input_attachment_index = 2, binding = 2) uniform subpassInput inAlbedo;
layout(input_attachment_index = 3, binding = 3) uniform subpassInput inMetallicRoughness;
layout(input_attachment_index = 4, binding = 4) uniform subpassInput inEmissive;

struct SpotLight {
    vec4  position;   // xyz = pos,  w = range
    vec4  direction;  // xyz = dir,  w = cos(innerCone)
    vec4  color;      // xyz = RGB,  w = intensity (lumens)
    float outerCone;
    float pad0; float pad1; float pad2;
};

struct PointLight {
    vec4 position;   // xyz = pos, w = range
    vec4 color;      // xyz = RGB, w = intensity (lumens)
};

layout(binding = 5) uniform LightUBO {
    mat4  lightSpaceMatrix;
    vec4  dirLightDir;
    vec4  dirLightColor;    // xyz = color, w = illuminance (lux)
    vec4  camPos;
    vec4  skyLight;         // xyz = sky color, w = sky irradiance (lux)
    float aperture;
    float shutterSpeed;
    float iso;
    int   spotLightCount;
    int   pointLightCount;
    int   useIBL;           // 1 = sample irradiance cubemap, 0 = hemisphere
    float iblIntensity;
    float _padLight;
    SpotLight  spotLights[8];
    PointLight pointLights[8];
} lights;

layout(binding = 6) uniform sampler2DShadow shadowMap;
layout(binding = 7) uniform samplerCube     irradianceMap;
layout(binding = 8) uniform samplerCube     prefilterMap;
layout(binding = 9) uniform sampler2D       brdfLUT;

const float MAX_REFLECTION_LOD = 4.0; // PREFILTER_MIP_LEVELS - 1

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

// -------------------------------------------------------
// Cook-Torrance BRDF
// -------------------------------------------------------
float D_GGX(vec3 N, vec3 H, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float denom  = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

float G_SchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float G_Smith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return G_SchlickGGX(NdotV, roughness) * G_SchlickGGX(NdotL, roughness);
}

vec3 F_Schlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Shared BRDF eval — returns (diffuse + specular) * irradiance * NdotL
vec3 EvalBRDF(vec3 N, vec3 V, vec3 L, vec3 albedo, float metallic,
              float roughness, vec3 F0, vec3 irradiance)
{
    vec3  H      = normalize(V + L);
    float NdotL  = max(dot(N, L), 0.0);
    if (NdotL == 0.0) return vec3(0.0);

    float D  = D_GGX(N, H, roughness);
    float G  = G_Smith(N, V, L, roughness);
    vec3  F  = F_Schlick(max(dot(H, V), 0.0), F0);

    vec3 specular  = (D * G * F) / (4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001);
    vec3 kD        = (vec3(1.0) - F) * (1.0 - metallic);
    vec3 diffuse   = kD * albedo / PI;

    return (diffuse + specular) * irradiance * NdotL;
}

// -------------------------------------------------------
// PCF shadow — 3x3 kernel, sampler2DShadow does comparison
// -------------------------------------------------------
float ShadowPCF(vec4 shadowCoord)
{
    // Perspective divide + remap XY to [0,1] UV
    // Z is already in [0,1] because lightSpaceMatrix includes the Vulkan Z scale-bias
    vec3 proj = shadowCoord.xyz / shadowCoord.w;
    proj.xy   = proj.xy * 0.5 + 0.5;

    // Outside the light frustum — treat as fully lit
    if (proj.z > 1.0 || proj.z < 0.0) return 1.0;

    const vec2 texelSize = vec2(1.0 / 2048.0);
    float shadow = 0.0;
    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            // texture() on sampler2DShadow returns 0 (shadowed) or 1 (lit)
            shadow += texture(shadowMap, vec3(proj.xy + vec2(x, y) * texelSize, proj.z));
        }
    }
    return shadow / 9.0;
}

// -------------------------------------------------------
void main()
{
    // Read G-Buffer.
    // position.w == 1.0 marks rendered geometry; 0.0 marks background/sky.
    vec4 positionSample = subpassLoad(inPosition);
    if (positionSample.w < 0.5)
    {
        outColor = vec4(0.529, 0.808, 0.922, 0.0); // sky; alpha=0 bypasses tonemapping exposure
        return;
    }

    vec3 worldPos = positionSample.xyz;
    vec3 N        = normalize(subpassLoad(inNormal).rgb * 2.0 - 1.0);
    vec4 albedoSample = subpassLoad(inAlbedo);

    vec3  albedo    = albedoSample.rgb;
    vec3  mr        = subpassLoad(inMetallicRoughness).rgb;
    float metallic  = mr.r;
    float roughness = max(mr.g, 0.04);
    float ao        = mr.b;

    vec3 V  = normalize(lights.camPos.xyz - worldPos);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 kD_approx = (vec3(1.0) - F_Schlick(max(dot(N, V), 0.0), F0)) * (1.0 - metallic);

    // -------------------------------------------------------
    // Directional light (with PCF shadows)
    // -------------------------------------------------------
    vec3 L             = normalize(-lights.dirLightDir.xyz);
    float illuminance  = lights.dirLightColor.w;
    vec3  dirIrradiance = lights.dirLightColor.rgb * illuminance;
    vec3  Lo           = EvalBRDF(N, V, L, albedo, metallic, roughness, F0, dirIrradiance);

    vec4  shadowCoord  = lights.lightSpaceMatrix * vec4(worldPos, 1.0);
    float shadowFactor = ShadowPCF(shadowCoord);
    Lo *= shadowFactor; // shadow only affects direct light

    // -------------------------------------------------------
    // Spot lights
    // -------------------------------------------------------
    for (int i = 0; i < lights.spotLightCount; i++)
    {
        SpotLight spot  = lights.spotLights[i];
        vec3  toLight   = spot.position.xyz - worldPos;
        float dist      = length(toLight);
        float range     = spot.position.w;
        if (dist > range) continue;

        vec3  Ls        = toLight / dist;

        float cosAngle  = dot(-Ls, normalize(spot.direction.xyz));
        float innerCone = spot.direction.w;
        float outerCone = spot.outerCone;
        float epsilon   = innerCone - outerCone;
        float spotFactor = clamp((cosAngle - outerCone) / epsilon, 0.0, 1.0);
        spotFactor = spotFactor * spotFactor;
        if (spotFactor <= 0.0) continue;

        float t          = dist / range;
        float rangeAtten = clamp(1.0 - t * t * t * t, 0.0, 1.0);
        rangeAtten       = rangeAtten * rangeAtten;
        float atten      = rangeAtten / (dist * dist + 1.0);

        vec3 spotIrradiance = spot.color.rgb * spot.color.w * atten * spotFactor;
        Lo += EvalBRDF(N, V, Ls, albedo, metallic, roughness, F0, spotIrradiance);
    }

    // -------------------------------------------------------
    // Point lights
    // -------------------------------------------------------
    for (int i = 0; i < lights.pointLightCount; i++)
    {
        PointLight pt = lights.pointLights[i];
        vec3  toLight = pt.position.xyz - worldPos;
        float dist    = length(toLight);
        float range   = pt.position.w;
        if (dist > range) continue;

        vec3  Lp = toLight / dist;
        float t  = dist / range;
        float rangeAtten = clamp(1.0 - t * t * t * t, 0.0, 1.0);
        rangeAtten = rangeAtten * rangeAtten;
        float atten = rangeAtten / (dist * dist + 1.0);

        vec3 ptIrradiance = pt.color.rgb * pt.color.w * atten;
        Lo += EvalBRDF(N, V, Lp, albedo, metallic, roughness, F0, ptIrradiance);
    }

    // -------------------------------------------------------
    // Ambient — IBL (diffuse + specular) or hemisphere fallback
    // -------------------------------------------------------
    // Cubemap was baked in Y-up (glTF) space; world is Z-up.
    // Rotate direction by -90° around X: (x, y, z) → (x, z, -y)
    vec3 ambient;
    if (lights.useIBL != 0)
    {
        // --- Diffuse IBL ---
        vec3 sampleDir  = vec3(N.x, N.z, -N.y);
        vec3 irradiance = texture(irradianceMap, sampleDir).rgb;

        vec3 F_IBL  = F_Schlick(max(dot(N, V), 0.0), F0);
        vec3 kD_IBL = (vec3(1.0) - F_IBL) * (1.0 - metallic);
        vec3 diffuseIBL = kD_IBL * albedo * irradiance;

        // --- Specular IBL (split-sum approximation) ---
        vec3 R    = reflect(-V, N);
        vec3 rDir = vec3(R.x, R.z, -R.y);
        float lod = roughness * MAX_REFLECTION_LOD;
        vec3 prefilteredColor = textureLod(prefilterMap, rDir, lod).rgb;
        vec2 envBRDF          = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
        vec3 specularIBL      = prefilteredColor * (F_IBL * envBRDF.x + envBRDF.y);

        ambient = (diffuseIBL + specularIBL) * ao * lights.iblIntensity;
    }
    else
    {
        // Hemisphere sky model: blue sky above, warm ground bounce below
        float upFactor      = dot(N, vec3(0.0, 0.0, 1.0)) * 0.5 + 0.5;
        vec3  skyIrradiance = lights.skyLight.rgb * lights.skyLight.a;
        vec3  groundBounce  = lights.dirLightColor.rgb * (lights.dirLightColor.w * 0.05);
        vec3  ambientIrr    = mix(groundBounce, skyIrradiance, upFactor);
        ambient = kD_approx * albedo / PI * ambientIrr * ao;
    }

    // -------------------------------------------------------
    // Combine + emissive
    // -------------------------------------------------------
    vec3 color  = ambient + Lo;
    color      += subpassLoad(inEmissive).rgb;

    outColor = vec4(color, 1.0);
}
