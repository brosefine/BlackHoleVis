
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;

layout (std140) uniform camera
{
    mat4 projectionView;
    mat4 projectionViewInverse;
    vec3 camPos;
};

uniform mat4 modelMatrix;

out vec2 uv;

void main()
{
    uv = vec2(aUV.x, 1-aUV.y);
    gl_Position = projectionView * modelMatrix * vec4(aPos, 1.0);
}