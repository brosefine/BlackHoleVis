
layout (location = 0) in vec3 aPos;

layout (std140) uniform camera
{
    mat4 projectionView;
    mat4 projectionViewInverse;
    vec3 camPos;
};

uniform mat4 modelMatrix;

out vec3 modelPos;

void main()
{
    modelPos = aPos;
    gl_Position = projectionView * modelMatrix * vec4(aPos, 1.0);
}