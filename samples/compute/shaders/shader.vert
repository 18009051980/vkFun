#version 450

layout(binding = 0) uniform UBO
{
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inVelocity;

layout(location = 0) out vec3 fragColor;


void main() {
    gl_PointSize = 5.0;
    gl_Position = vec4(inPosition, 1.0);
    fragColor = inColor;
}