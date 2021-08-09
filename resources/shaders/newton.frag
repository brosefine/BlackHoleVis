
#ifdef SKY
uniform samplerCube cubeMap;
#endif //SKY
uniform float stepSize;
uniform float forceWeight;

in vec3 cameraPos;
in vec3 worldPos;
out vec4 FragColor;

const float rs = 1.0;

vec3 newton(vec3 pos) {
    return -forceWeight * normalize(pos)/dot(pos, pos);
}

void main() {
    
    FragColor = vec4(0,0,0,0);
    vec4 diskColor;

    vec3 viewDir = normalize(worldPos - cameraPos);

    #ifndef DISK
    float dotP = dot(normalize(-cameraPos), viewDir);
    float dist = length(dotP * length(cameraPos) * viewDir + cameraPos);
    #ifdef RAYDIRTEST
    // stop if lightray is pointing directly to black hole
    if(dotP >= 0 && abs(dist) <= rs) return;
    #endif //RAYDIRTEST

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
        if(diskIntersect(lightPos, -stp*lightVel, diskColor)){
            FragColor.rgb += (1.0 - FragColor.a) * diskColor.rgb * diskColor.a;
            FragColor.a += diskColor.a;
            if(FragColor.a > 0.99) return;
        }
        #endif //DISK
    }

    #endif //FIRSTRK4

    // simple loop for now
    for(int i = 0; i < 100; ++i) {
        
        if(length(lightPos) <= rs){
        #ifdef CHECKEREDHOR
            vec4 horizonColor;
            horizonIntersect (lightPos, -lightVel*stepSize, rs, horizonColor);
            FragColor.rgb += (1.0 - FragColor.a) * horizonColor.rgb;
        #endif
            return;
        }
        
        lightVel = normalize(lightVel + newton(lightPos));
        lightPos += lightVel * stepSize;

        #ifdef DISK
        if (diskIntersect(lightPos, -stepSize*lightVel, diskColor)) {
            FragColor.rgb += (1.0 - FragColor.a) * diskColor.rgb * diskColor.a;
            FragColor.a += diskColor.a;
            if(FragColor.a > 0.99) return;
        }
        #endif //DISK
    }

    #ifdef SKY
    FragColor.rgb += (1.0 - FragColor.a) * texture(cubeMap, lightVel).rgb;
    #else
    FragColor.rgb += (1.0 - FragColor.a) * abs(normalize(lightVel));
    #endif //SKY
    FragColor.a = 1.0;
}