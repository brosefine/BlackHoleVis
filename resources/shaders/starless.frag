
#ifdef SKY
uniform samplerCube cubeMap;
#endif //SKY
uniform float stepSize;
uniform float potentialCoefficient;
/* Note on potentialCoefficient
* in starless: 0 >= potentialCoefficient >= -1.5
* here, value is 0 <= potentialCoefficient <= 1.5
* because using distance (blackHole - lightPos)
*/


in vec3 cameraPos;
in vec3 worldPos;
out vec4 FragColor;

const float rs = 1.0;

vec3 starless(vec3 pos, float h2){
    return -potentialCoefficient * h2 * pos / pow(dot(pos, pos), 2.5);
}

void main() {
    
    FragColor = vec4(0,0,0,0);
    vec4 diskColor;
    float transparency = 1.0;

    vec3 viewDir = normalize(worldPos - cameraPos);
  
    #ifndef DISK
    float dotP = dot(normalize(-cameraPos), viewDir);
    float dist = length(dotP * length(cameraPos) * viewDir + cameraPos);
    // stop if lightray is pointing directly to black hole
    #ifdef RAYDIRTEST
    if(dotP >= 0 && abs(dist) <= rs) return;
    #endif //RAYDIRTEST

    #ifdef EHSIZE
    // theoretical apparent EH size
    if(dotP >= 0 && abs(dist) <= 2.6 * rs) {
        FragColor = vec4(0.5,0.5,0.5,1);
        return;
    }
    #endif // EHSIZE
    #endif
    
    vec3 lightPos = cameraPos;
    vec3 lightVel = viewDir;
    vec3 lightup = cross(cameraPos, lightVel);
    float h2 = dot(lightup, lightup);

    #ifdef FIRSTRK4
    if(length(lightPos) > rs) {
        float stp = length(lightPos) - rs;
        vec3 k1 = starless(lightPos, h2);
        vec3 k2 = starless(lightPos + 0.5 * stp * k1, h2);
        vec3 k3 = starless(lightPos + 0.5 * stp * k2, h2);
        vec3 k4 = starless(lightPos + stp * k3, h2);

        lightVel = normalize(lightVel + stp/6.0 * (k1 + 2*k2 + 2*k3 + k4));
        lightPos += stp * lightVel;

        #ifdef DISK
        if(diskIntersect(lightPos, -stp*lightVel, diskColor)){
            FragColor.rgb += transparency * diskColor.rgb * diskColor.a;
            transparency *= (1.0 - diskColor.a);
            FragColor.a = 1.0 - transparency;
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
            FragColor.rgb += transparency * horizonColor.rgb;
        #endif
            return;
        }

        lightVel = normalize(lightVel + starless(lightPos, h2) * stepSize);
        lightPos += lightVel * stepSize;
         
        #ifdef DISK
        if (diskIntersect(lightPos, -stepSize*lightVel, diskColor)) {
            FragColor.rgb += transparency * diskColor.rgb * diskColor.a;
            transparency *= (1.0 - diskColor.a);
            FragColor.a = 1.0 - transparency;
            if(FragColor.a > 0.99) return;
        }
        #endif //DISK
    }

    #ifdef SKY
    FragColor.rgb += transparency * texture(cubeMap, lightVel).rgb;
    #else
    FragColor.rgb += transparency * abs(normalize(lightVel));
    #endif //SKY
    FragColor.a = 1.0;
}