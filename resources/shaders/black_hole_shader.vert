
layout (location = 0) in vec3 aPos;

layout (std140) uniform camera
{
    mat4 projectionViewInverse;
    mat4 projectionInverse;
    vec3 camPos;
};

out vec3 viewDir;
out vec3 cameraPos;

void main()
{
    viewDir = normalize(vec3(projectionInverse * vec4(aPos, 1.0)));
    cameraPos = camPos;

    gl_Position = vec4(aPos, 1.0);
}