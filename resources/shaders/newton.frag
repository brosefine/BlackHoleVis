
#ifdef SKY
uniform samplerCube cubeMap;
#endif //SKY
uniform float stepSize;
uniform float forceWeight;

in vec3 cameraPos;
in vec3 worldPos;
out vec4 FragColor;

const float rs = 1.0;
float accretionMin = 4.0;
float accretionMax = 8.0;

vec3 newton(vec3 pos) {
    return -forceWeight * normalize(pos)/dot(pos, pos);
}

void main() {
    
    FragColor = vec4(0,0,0,1);

    vec3 viewDir = normalize(worldPos - cameraPos);

    #ifndef DISK
    // stop if lightray is pointing directly to black hole
    float dotP = dot(normalize(-cameraPos), viewDir);
    float dist = length(dotP * length(cameraPos) * viewDir + cameraPos);
    if(dotP >= 0 && abs(dist) <= rs) return;

    #ifdef EHSIZE
    // theoretical apparent EH size
    if(dotP >= 0 && abs(dist) <= 2.6 * rs) {
        FragColor = vec4(0.5,0.5,0.5,1);
        return;
    }
    #endif // EHSIZE
    #endif //DISK
    
    vec3 lightPos = cameraPos;
    vec3 lightVel = viewDir;

    #ifdef FIRSTRK4
    if(length(lightPos) > rs) {
        float stp = length(lightPos) - rs;
        vec3 k1 = newton(lightPos);
        vec3 k2 = newton(lightPos + 0.5 * stp * k1);
        vec3 k3 = newton(lightPos + 0.5 * stp * k2);
        vec3 k4 = newton(lightPos + stp * k3);

        lightVel = normalize(lightVel + stp/6.0 * (k1 + 2*k2 + 2*k3 + k4));
        lightPos += stp * lightVel;

        #ifdef DISK
        vec3 prevPos = lightPos - lightVel * stp;
        if((prevPos.y < 0 && lightPos.y > 0) ||(prevPos.y > 0 && lightPos.y < 0)){
            if(lightVel.y == 0.0) { FragColor = vec4(1,1,1,1); return; }
            vec3 diskHit = lightPos - lightPos.y * lightVel / lightVel.y;
            if(length(diskHit) < accretionMax && length(diskHit) > accretionMin) {
                float heat = (length(diskHit) - accretionMin)/(accretionMax-accretionMin);
                FragColor = vec4(1, 1.0 - heat, 0.7 - heat, 1);                
                //FragColor = vec4(1,1,1,1);
                return;
            }
        }
        #endif //DISK
    }

    #endif //FIRSTRK4

    // simple loop for now
    for(int i = 0; i < 100; ++i) {
        
        if(length(lightPos) <= rs) return;
        #ifdef RAYDIRTEST
        float dotP = dot(normalize(-lightPos), normalize(lightVel));
        float dist = length(dotP * length(lightPos) * normalize(lightVel) + lightPos);
        if(dotP >= 0 && abs(dist) <= rs) return;
        #endif
        
        lightVel = normalize(lightVel + newton(lightPos));
        lightPos += lightVel * stepSize;

        #ifdef DISK
        vec3 prevPos = lightPos - lightVel * stepSize;
        if((prevPos.y < 0 && lightPos.y > 0) ||(prevPos.y > 0 && lightPos.y < 0)){
            if(lightVel.y == 0.0) { FragColor = vec4(1,1,1,1); return; }
            vec3 diskHit = lightPos - lightPos.y * lightVel / lightVel.y;
            if(length(diskHit) < accretionMax && length(diskHit) > accretionMin) {
                float heat = (length(diskHit) - accretionMin)/(accretionMax-accretionMin);
                FragColor = vec4(1, 1.0 - heat, 0.7 - heat, 1);
                //FragColor = vec4(1,1,1,1);
                return;
            }
        }
        #endif //DISK
    }

    #ifdef SKY
    FragColor = texture(cubeMap, lightVel);
    #else
    FragColor = vec4(abs(normalize(lightVel)), 1.0);
    #endif //SKY
}