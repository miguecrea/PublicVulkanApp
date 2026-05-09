#version 450

layout(set = 1, binding = 0) uniform sampler2D albedoMap;
layout(set = 1, binding = 1) uniform sampler2D normalMap;
layout(set = 1, binding = 2) uniform MaterialUBO {
    vec4  baseColorFactor;
    vec4  emissiveFactor;
    float metallicFactor;
    float roughnessFactor;
    float hasNormalMap;
    float hasMetallicRoughness;
    float hasAO;
    float hasEmissive;
    float alphaMask;
    float alphaCutoff;
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
layout(location = 4) out vec4 outEmissive;

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

    // Albedo (clean, no AO or emissive baked in)
    outAlbedo = vec4(albedoSample.rgb, albedoSample.a);

    // Metallic/Roughness + AO in B channel (glTF: factors multiply the texture values)
    float metallic  = material.metallicFactor;
    float roughness = material.roughnessFactor;
    if (material.hasMetallicRoughness > 0.5)
    {
        vec4 mrSample = texture(metallicRoughnessMap, fragTexCoord);
        roughness *= mrSample.g;
        metallic  *= mrSample.b;
    }

    float ao = 1.0;
    if (material.hasAO > 0.5)
        ao = texture(aoMap, fragTexCoord).r;

    outMetallicRoughness = vec4(metallic, roughness, ao, 0.0);

    // Emissive (separate output, unlit contribution)
    // glTF: emissive = texture * factor, or just factor when no texture
    vec3 emissive = material.emissiveFactor.rgb;
    if (material.hasEmissive > 0.5)
        emissive *= texture(emissiveMap, fragTexCoord).rgb;
    outEmissive = vec4(emissive, 0.0);
}
