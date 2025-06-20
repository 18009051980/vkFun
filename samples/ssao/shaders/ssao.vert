#version 450

layout(binding = 0) uniform UBO
{
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout (binding = 1) uniform Time
{
    float t;
} time;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in vec3 inNormal;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo. model * vec4(inPosition, 1.0);
    // float cycleTime = mod(time.t, 3.14);
    outPosition = (ubo.model * vec4(inPosition, 1.0));
    outNormal = mat4(transpose(inverse(ubo.model))) * vec4(inNormal, 1);
}