
#ifdef SKY
uniform samplerCube cubeMap;
#endif //SKY

in vec3 cameraPos;
in vec3 worldPos;
out vec4 FragColor;

float accretionMin = 4.0;
float accretionMax = 8.0;

void main() {

    vec3 viewDir = normalize(worldPos - cameraPos);
    vec3 blackHoleVec = -cameraPos;
    
    if(length(blackHoleVec) <= 1) {
        FragColor = vec4(1,1,1,1);
        return;
    }

    float dotP = dot(normalize(blackHoleVec), viewDir);
    float dist = length(dotP * length(blackHoleVec) * viewDir - blackHoleVec);
    bool blackHoleHit = dotP > 0 && abs(dist) < 1;

    if(!blackHoleHit) {
        #ifdef SKY
        FragColor = texture(cubeMap, viewDir);
        #else
        FragColor = vec4(abs(viewDir),1);
        #endif //SKY
    } else {
        FragColor = vec4(0,0,0,1);
    }
    
    #ifdef DISK
    if(sign(cameraPos.y) != sign(viewDir.y)) {
        vec3 diskHit = cameraPos - cameraPos.y * viewDir / viewDir.y;
        if(!(blackHoleHit && length(diskHit - cameraPos) > length(cameraPos)) && length(diskHit) < accretionMax && length(diskHit) > accretionMin) {
            float heat = (length(diskHit) - accretionMin)/(accretionMax-accretionMin);
            FragColor = vec4(1, 1.0 - heat, 0.7 - heat, 1);                
        }
    }
    #endif //DISK

}