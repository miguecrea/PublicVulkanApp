#version 450

layout(set = 1, binding = 0) uniform sampler2D albedoMap;
layout(set = 1, binding = 1) uniform sampler2D normalMap;
layout(set = 1, binding = 2) uniform MaterialUBO {
    vec4  baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float hasNormalMap;
    float hasMetallicRoughness;
    float hasAO;
    float hasEmissive;
    float alphaMask;
    float alphaCutoff;
    float padding[2];
} material;
layout(set = 1, binding = 3) uniform sampler2D metallicRoughnessMap;
layout(set = 1, binding = 4) uniform sampler2D aoMap;
layout(set = 1, binding = 5) uniform sampler2D emissiveMap;

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in mat3 fragTBN;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outAlbedo;
layout(location = 3) out vec4 outMetallicRoughness;

void main()
{
    // Alpha cutout
    vec4 albedoSample = texture(albedoMap, fragTexCoord) * material.baseColorFactor;
    if (material.alphaMask > 0.5 && albedoSample.a < material.alphaCutoff)
        discard;

    outPosition = vec4(fragWorldPos, 1.0);

    // Normal mapping
    vec3 N;
    if (material.hasNormalMap > 0.5)
    {
        vec3 tangentNormal = texture(normalMap, fragTexCoord).xyz * 2.0 - 1.0;
        N = normalize(fragTBN * tangentNormal);
    }
    else
    {
        N = normalize(fragTBN[2]);
    }
    outNormal = vec4(N * 0.5 + 0.5, 0.0);

    // Albedo + AO
    vec3 albedo = albedoSample.rgb;
    if (material.hasAO > 0.5)
        albedo *= texture(aoMap, fragTexCoord).r;

    // Add emissive
    if (material.hasEmissive > 0.5)
        albedo += texture(emissiveMap, fragTexCoord).rgb;

    outAlbedo = vec4(albedo, albedoSample.a);

    // Metallic/Roughness
    float metallic  = material.metallicFactor;
    float roughness = material.roughnessFactor;
    if (material.hasMetallicRoughness > 0.5)
    {
        vec4 mrSample = texture(metallicRoughnessMap, fragTexCoord);
        roughness = mrSample.g;
        metallic  = mrSample.b;
    }
    outMetallicRoughness = vec4(metallic, roughness, 0.0, 0.0);
}