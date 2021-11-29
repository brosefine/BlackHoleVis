
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

layout (std140) uniform camera
{
    mat4 projectionViewInverse;
    mat4 projectionInverse;
    vec3 camPos;
};

out vec3 cameraPos;
out vec3 viewDir;
out vec2 TexCoords;


void main()
{
    viewDir = normalize(vec3(projectionInverse * vec4(aPos, 1.0)));
    cameraPos = camPos;
    TexCoords = aTexCoords;    

    gl_Position = vec4(aPos, 1.0);
}