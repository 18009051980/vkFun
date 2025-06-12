#version 450


layout(binding = 2) uniform sampler2D texSampler;

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inNormal;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;


void main() {
    outPosition = outPosition;
    outNormal = inNormal;
} 