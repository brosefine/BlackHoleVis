
layout (location = 0) in vec3 aPos;

layout (std140) uniform camera
{
    mat4 projectionView;
    mat4 projectionViewInverse;
    vec3 camPos;
};

out vec2 texPos;

void main()
{
    texPos = aPos.xy; 
    gl_Position = vec4(aPos, 1.0);
}