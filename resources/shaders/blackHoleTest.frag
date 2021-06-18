#version 330 core

uniform vec3 cameraPos;

layout (std140) uniform blackHole
{
    vec3 blackHolePos;
    float blackHoleMass;
    float blackHoleRad;
};

in vec3 worldPos;
out vec4 FragColor;

void main() {

    vec3 viewDir = normalize(worldPos - cameraPos);
    vec3 blackHoleVec = blackHolePos - cameraPos;
    
    if(length(blackHoleVec) <= blackHoleRad) {
        FragColor = vec4(1,1,1,1);
        return;
    }

    float dotP = dot(normalize(blackHoleVec), viewDir);
    float dist = length(dotP * length(blackHoleVec) * viewDir - blackHoleVec);

    if(dotP < 0 || abs(dist) > blackHoleRad) {
        FragColor = vec4(abs(viewDir),1);
    } else {
        FragColor = vec4(0,0,0,1);
    }
}