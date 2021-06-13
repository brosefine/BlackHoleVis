#version 330 core

layout (location = 0) in vec3 aPos;

uniform mat4 projectionViewInverse;

out vec3 worldPos;

void main()
{
    vec4 invPos = projectionViewInverse * vec4(aPos, 1.0);
    worldPos = invPos.xyz / invPos.w;
    gl_Position = vec4(aPos, 1.0);
}