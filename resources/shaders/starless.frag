
#ifdef SKY
layout(binding = 0) uniform samplerCube cubeMap;
#endif //SKY
uniform float stepSize;
uniform float forceWeight = 1.0;
uniform float M;
uniform vec4 sphere = vec4(0, 0, 0, 0);

in vec3 cameraPos;
in vec3 worldPos;
out vec4 FragColor;

float rs = 2.0 * M;

vec3 starless(vec3 pos, float h2){
    return -3 * h2 * M * pos / pow(dot(pos, pos), 2.5);
}

vec3 rk4(vec3 pos, float h2, float step) {
    vec3 k1 = starless(pos, h2);
    vec3 k2 = starless(pos + 0.5 * step * k1, h2);
    vec3 k3 = starless(pos + 0.5 * step * k2, h2);
    vec3 k4 = starless(pos + step * k3, h2);

    return (k1 + 2*k2 + 2*k3 + k4) / 6.0;
}

void main() {
    
    FragColor = vec4(0,0,0,0);
    #ifdef DISK
    vec4 diskColor;
    #endif
    #ifdef CHECKEREDHOR
    vec4 horizonColor;
    #endif

    vec3 viewDir = normalize(worldPos - cameraPos);
  
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

    // simple loop for now
    float step = stepSize;
    for(int i = 0; i < 100; ++i) {

        #ifdef ADPTSTEP
        step = max(0.0001, stepSize * (length(lightPos) - rs));
        #endif //ADPTSTEP

        #ifdef FIRSTRK4
        vec3 acc = rk4(lightPos, h2, step);
        #else
        vec3 acc = starless(lightPos, h2);
        #endif
        
        #ifdef ERLYTERM
        if (dot(lightPos, lightVel) > 0 && 
            length(acc*step) <= length(lightVel) * 0.001) {
            break;
        }
        #endif //ERLYTERM

        lightVel += forceWeight * acc * step;
        lightPos += lightVel * step;
         
        #ifdef CHECKEREDHOR
        if(horizonIntersect (lightPos, -lightVel*step, rs, horizonColor)){
            FragColor.rgb += (1.0 - FragColor.a) * horizonColor.rgb;
        #else 
        if(length(lightPos) <= rs){
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

        #ifdef SPHERE
        #ifdef CHECKEREDHOR
        if (horizonIntersect (lightPos - sphere.xyz, -lightVel*step, sphere.w, horizonColor))
        {
            FragColor.rgb += (1.0 - FragColor.a) * horizonColor.bgr;
        #else
        if(length(lightPos - sphere.xyz) <= sphere.w){
            FragColor.rgb += (1.0-FragColor.a) * vec3(1);
            FragColor.a = 1.0;
        #endif //CHECKEREDHOR
            return;
        }
        #endif //SPHERE
    }

    #ifdef SKY
    FragColor.rgb += (1.0-FragColor.a) * texture(cubeMap, lightVel).rgb;
    #else
    FragColor.rgb += (1.0-FragColor.a) * abs(normalize(lightVel));
    #endif //SKY
    FragColor.a = 1.0;
}