
#ifdef SKY
layout(binding = 0) uniform samplerCube cubeMap;
#endif //SKY
uniform float stepSize;
uniform float M;

in vec3 cameraPos;
in vec3 worldPos;
out vec4 FragColor;

vec3 starless(vec3 pos, float h2){
    return -3 * h2 * M * pos / pow(dot(pos, pos), 2.5);
}

void main() {
    
    FragColor = vec4(0,0,0,0);
    vec4 diskColor;

    vec3 viewDir = normalize(worldPos - cameraPos);
  
    float rs = 2.0 * M;
    float dotP = dot(normalize(-cameraPos), viewDir);
    float dist = length(dotP * length(cameraPos) * viewDir + cameraPos);
    // stop if lightray is pointing directly to black hole
    #ifndef DISK
    #ifdef RAYDIRTEST
    if(dotP >= 0 && abs(dist) <= rs) return;
    #endif //RAYDIRTEST
    #endif

    #ifdef EHSIZE
    // theoretical apparent EH size
    if(dotP >= 0 && abs(dist) <= 2.6 * rs) {
        FragColor = vec4(0.5,0.5,0.5,0.5);
        //return;
    }
    #endif // EHSIZE
    
    vec3 lightPos = cameraPos;
    vec3 lightVel = viewDir;
    vec3 lightup = cross(cameraPos, lightVel);
    float h2 = dot(lightup, lightup);

    #ifdef FIRSTRK4
    if(length(lightPos) > 100.0) {
        float stp = length(lightPos) - 100.0;
        vec3 k1 = starless(lightPos, h2);
        vec3 k2 = starless(lightPos + 0.5 * stp * k1, h2);
        vec3 k3 = starless(lightPos + 0.5 * stp * k2, h2);
        vec3 k4 = starless(lightPos + stp * k3, h2);

        lightVel += stp/6.0 * (k1 + 2*k2 + 2*k3 + k4);
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
    float step = stepSize;
    for(int i = 0; i < 100; ++i) {

        #ifdef ADPTSTEP
        step = max(0.05, min(10.0, stepSize * (length(lightPos)-1.0)));
        #endif //ADPTSTEP


        vec3 acc = starless(lightPos, h2);
        #ifdef ERLYTERM
        if (dot(-lightPos, lightVel) < 0 && 
            length(acc*step) <= length(lightVel) * 0.001) {
            break;
        }
        #endif //ERLYTERM
        lightVel += acc * step;
        lightPos += lightVel * step;
         
        if(length(lightPos) <= rs){
        #ifdef CHECKEREDHOR
            vec4 horizonColor;
            horizonIntersect (lightPos, -lightVel*step, rs, horizonColor);
            FragColor.rgb += (1.0 - FragColor.a) * horizonColor.rgb;
        #endif //CHECKEREDHOR
            return;
        }

        #ifdef DISK
        if (diskIntersect(lightPos, -step*lightVel, diskColor)) {
            FragColor.rgb += (1.0 - FragColor.a) * diskColor.rgb * diskColor.a;
            FragColor.a += diskColor.a;
            if(FragColor.a > 0.99) return;
        }
        #endif //DISK
    }

    #ifdef SKY
    FragColor.rgb += (1.0-FragColor.a) * texture(cubeMap, lightVel).rgb;
    #else
    FragColor.rgb += (1.0-FragColor.a) * abs(normalize(lightVel));
    #endif //SKY
    FragColor.a = 1.0;
}