#version 450

layout (binding = 0) uniform Ubo
{
    mat4 mvp;
} ubo;


layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in vec3 inNormal;

layout (location = 0) out vec4 fragColor;
void main()
{ 
    gl_Position = ubo.mvp * vec4(inPosition, 1.f);
    fragColor = vec4(inColor, 0.5f);
}