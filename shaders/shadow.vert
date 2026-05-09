#version 450

layout(location = 0) in vec3 inPos;
// Other vertex attributes are not needed for shadow depth pass.
// The vertex binding must still describe the full Vertex stride so the
// hardware reads offsets correctly; we just don't use the extras here.
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec4 inTangent;

layout(push_constant) uniform PC
{
    mat4 lightSpaceMatrix;   // offset  0, size 64
    mat4 model;              // offset 64, size 64
} pc;

void main()
{
    gl_Position = pc.lightSpaceMatrix * pc.model * vec4(inPos, 1.0);
}
