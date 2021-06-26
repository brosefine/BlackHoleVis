#version 330 core

//uniform vec3 cameraPos;

layout (std140) uniform blackHole
{
    vec3 blackHolePos;
    float blackHoleMass;
    float blackHoleRad;
};

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

// speed of light in our units 
// (see Blackhole.h)
float c = 3.0;

vec3 starless(vec3 lightPos, vec3 lightVel, vec3 dist){
    return blackHoleMass * potentialCoefficient * dist / pow(dot(dist, dist), 2.5);
}

void main() {
    
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
        return;
    }

    #ifdef EHSIZE
    // theoretical apparent EH size
    
    if(dotP >= 0 && abs(dist) <= 2.6 * blackHoleRad) {
        FragColor = vec4(0.5,0.5,0.5,1);
        return;
    }
    #endif // EHSIZE
    

    // simple loop for now
    vec3 lightPos = cameraPos;
    vec3 lightVel = c * viewDir;
    vec3 lightup = cross(blackHoleVec, lightVel);
    float h2 = dot(lightup, lightup);
    for(int i = 0; i < 100; ++i) {
        
        blackHoleVec = blackHolePos - lightPos;
        #ifndef RAYDIRTEST
        if(length(blackHoleVec) <= blackHoleRad) return;
        #else
        float dotP = dot(normalize(blackHoleVec), normalize(lightVel));
        float dist = length(dotP * length(blackHoleVec) * normalize(lightVel) - blackHoleVec);
        if(dotP >= 0 && abs(dist) <= blackHoleRad) {
            FragColor = vec4(0,1,0,1);
            return;
        }
        #endif
        
        lightPos += lightVel * stepSize;
        lightVel += starless(lightPos, lightVel, blackHoleVec) * stepSize;
    }

    FragColor = vec4(abs(normalize(lightVel)), 1.0);
}