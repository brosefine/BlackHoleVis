
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

layout (std140) uniform camera
{
    mat4 projectionView;
    mat4 projectionViewInverse;
    vec3 camPos;
};

out vec3 worldPos;
out vec3 cameraPos;
out vec2 TexCoords;

void main()
{
    vec4 invPos = projectionViewInverse * vec4(aPos, 1.0);
    
    worldPos = invPos.xyz / invPos.w;
    cameraPos = camPos;
    TexCoords = aTexCoords;    

    gl_Position = vec4(aPos, 1.0);
}