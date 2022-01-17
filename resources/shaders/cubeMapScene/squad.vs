
layout (location = 0) in vec3 aPos;

layout (std140) uniform camera
{
    mat4 projectionView;
    mat4 projectionViewInverse;
    vec3 camPos;
};

out vec3 viewDir;

void main()
{
    vec4 invPos = projectionViewInverse * vec4(aPos, 1.0);
    
    viewDir = invPos.xyz / invPos.w - camPos;

    gl_Position = vec4(aPos.xy, 0.99, 1.0);
}