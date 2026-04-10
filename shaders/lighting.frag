#version 450

layout(input_attachment_index = 0, binding = 0) uniform subpassInput inPosition;
layout(input_attachment_index = 1, binding = 1) uniform subpassInput inNormal;
layout(input_attachment_index = 2, binding = 2) uniform subpassInput inAlbedo;
layout(input_attachment_index = 3, binding = 3) uniform subpassInput inMetallicRoughness;
layout(input_attachment_index = 4, binding = 4) uniform subpassInput inEmissive;

layout(binding = 5) uniform LightUBO {
    vec4 dirLightDir;
    vec4 dirLightColor;
    vec4 camPos;
    float aperture;
    float shutterSpeed;
    float iso;
    float padding;
} lights;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

// -------------------------------------------------------
// Cook-Torrance BRDF functions
// -------------------------------------------------------

// Normal Distribution Function - GGX/Trowbridge-Reitz
float D_GGX(vec3 N, vec3 H, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float NdotH  = max(dot(N, H), 0.0);

    float NdotH2 = NdotH * NdotH;
    float denom  = NdotH2 * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

// Geometry Function - Smith with Schlick-GGX
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
    NdotL = clamp(NdotL, 0.0, 1.0);

    return G_SchlickGGX(NdotV, roughness) * G_SchlickGGX(NdotL, roughness);
}

// Fresnel - Schlick approximation
vec3 F_Schlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// -------------------------------------------------------
void main()
{
    // Read G-Buffer
    vec3 worldPos  = subpassLoad(inPosition).rgb;
    vec3 N         = normalize(subpassLoad(inNormal).rgb * 2.0 - 1.0); // decode from [0,1]

    if (length(N) < 0.1)
    {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec4 albedo = subpassLoad(inAlbedo);
    if (albedo.a == 0.0)
    {
        outColor = vec4(0.529, 0.808, 0.922, 1.0);
        return;
    }

    vec3 mr = subpassLoad(inMetallicRoughness).rgb;
    float metallic  = mr.r;
    float roughness = max(mr.g, 0.04);
    float ao        = mr.b;

    vec3 V = normalize(lights.camPos.xyz - worldPos);
    vec3 L = normalize(-lights.dirLightDir.xyz);
    vec3 H = normalize(V + L);

    // F0 = base reflectivity
    vec3 F0 = mix(vec3(0.04), albedo.rgb, metallic);

    // Cook-Torrance specular BRDF
    float D   = D_GGX(N, H, roughness);
    float G   = G_Smith(N, V, L, roughness);
    vec3  F   = F_Schlick(max(dot(H, V), 0.0), F0);

    vec3 numerator   = D * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular    = numerator / denominator;

    // Diffuse - metals have no diffuse
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
    vec3 diffuse = kD * albedo.rgb / PI;

    float NdotL     = max(dot(N, L), 0.0);
    float illuminance = lights.dirLightColor.w;
    vec3 irradiance   = lights.dirLightColor.xyz * illuminance;
    vec3 Lo = (diffuse + specular) * irradiance * NdotL;

    // Ambient - approximate indirect as fraction of direct irradiance
    // (placeholder until IBL is added)
    vec3 ambientIrradiance = irradiance * 0.3;
    vec3 ambient = kD * albedo.rgb / PI * ambientIrradiance * ao;

    vec3 color = ambient + Lo;

    // Add emissive (unlit, post-lighting)
    vec3 emissive = subpassLoad(inEmissive).rgb;
    color += emissive;

    outColor = vec4(color, 1.0);
}
