#version 450


layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;

layout (binding = 0) uniform Mvp
{
    mat4 mvp;
} matrix;

void main()
{
    gl_Position = matrix.mvp * vec4(inPosition, 1.f);
}