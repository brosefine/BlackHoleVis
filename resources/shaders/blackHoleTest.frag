#version 330 core

#ifdef SKY
uniform samplerCube cubeMap;
#endif //SKY

in vec3 cameraPos;
in vec3 worldPos;
out vec4 FragColor;

void main() {


    vec3 viewDir = normalize(worldPos - cameraPos);
    vec3 blackHoleVec = -cameraPos;
    
    if(length(blackHoleVec) <= 1) {
        FragColor = vec4(1,1,1,1);
        return;
    }

    float dotP = dot(normalize(blackHoleVec), viewDir);
    float dist = length(dotP * length(blackHoleVec) * viewDir - blackHoleVec);

    if(dotP < 0 || abs(dist) > 1) {
        #ifdef SKY
        FragColor = texture(cubeMap, viewDir);
        #else
        FragColor = vec4(abs(viewDir),1);
        #endif //SKY
    } else {
        FragColor = vec4(0,0,0,1);
    }
}