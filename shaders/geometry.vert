#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 model; // kept for compatibility but not used
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 normalMatrix;  // transpose(inverse(model)), precomputed on CPU
} push;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec4 inTangent;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out mat3 fragTBN;

void main()
{
    vec4 worldPos = push.model * vec4(inPos, 1.0);
    gl_Position   = ubo.proj * ubo.view * worldPos;
    fragWorldPos  = worldPos.xyz;
    fragTexCoord  = inTexCoord;

    mat3 nm = mat3(push.normalMatrix);
    vec3 N = normalize(nm * inNormal);
    vec3 T = normalize(nm * inTangent.xyz);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T) * inTangent.w;
    fragTBN = mat3(T, B, N);
}