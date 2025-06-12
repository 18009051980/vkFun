#version 450

layout (binding = 0) uniform MatrixUbo
{
    vec3 viewPos;
    mat4 model;
    mat4 mvp;
    mat4 lightMvp;
} matrix;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

layout (location = 0) out vec3 outPosition;
layout (location = 1) out vec3 outNormal;


void main()
{
    gl_Position = matrix.mvp * vec4(position, 1.f);
    outPosition = (matrix.model * vec4(position, 1.f)).xyz;
    outNormal = transpose(inverse(mat3(matrix.model))) * normal;
}