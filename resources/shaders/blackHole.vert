#version 330 core

layout (location = 0) in vec3 aPos;

layout (std140) uniform camera
{
    mat4 projectionViewInverse;
    vec3 camPos;
};

uniform mat4 help;

out vec3 worldPos;
out vec3 cameraPos;

void main()
{
    vec4 invPos = projectionViewInverse * vec4(aPos, 1.0);
    
    worldPos = invPos.xyz / invPos.w;
    cameraPos = camPos;

    gl_Position = vec4(aPos, 1.0);
}