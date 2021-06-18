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

// speed of light in our units 
// (see Blackhole.h)
float c = 3.0;

vec3 newton(vec3 lightPos, vec3 lightVel, vec3 dist){
    vec3 force = normalize(dist) * blackHoleMass * 8.5e-4 / dot(dist, dist); 
    return c * normalize(lightVel + force);
}

void main() {
    
    //FragColor = vec4(1,1,1,1);
    FragColor = vec4(0,0,0,1);

    vec3 viewDir = normalize(worldPos - cameraPos);
    vec3 blackHoleVec = blackHolePos - cameraPos;
    
    if(length(blackHoleVec) <= blackHoleRad) {
        return;
    }

    // stop if lightray is pointing directly to black hole
    
    float dotP = dot(normalize(blackHoleVec), viewDir);
    float dist = length(dotP * length(blackHoleVec) * viewDir - blackHoleVec);
    if(dotP >= 0 && abs(dist) <= blackHoleRad) {
        //FragColor = vec4(0,0,0,1);
        return;
    }
    // theoretical apparent EH size
    /*
    if(dotP >= 0 && abs(dist) <= 2.6 * blackHoleRad) {
        FragColor = vec4(0.5,0.5,0.5,1);
        return;
    }
    */
    
    //FragColor = vec4(abs(normalize(viewDir)), 1.0); return;
    

    // simple loop for now
    vec3 lightPos = cameraPos;
    vec3 lightVel = c * viewDir;
    for(int i = 0; i < 100; ++i) {
        
        blackHoleVec = blackHolePos - lightPos;
        if(length(blackHoleVec) <= blackHoleRad) return;
        /*
        float dotP = dot(normalize(blackHoleVec), normalize(lightVel));
        float dist = length(dotP * length(blackHoleVec) * normalize(lightVel) - blackHoleVec);
        if(dotP >= 0 && abs(dist) <= blackHoleRad) {
            return;
        }*/
        
        lightPos += lightVel * 2.5;
        lightVel = newton(lightPos, lightVel, blackHoleVec);
    }

    FragColor = vec4(abs(normalize(lightVel)), 1.0);
}