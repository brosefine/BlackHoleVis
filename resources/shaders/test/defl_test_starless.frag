
layout(binding = 2) uniform samplerCube cubeMap;
uniform float stepSize = 0.2;
uniform float M = 0.5;
uniform vec3 cam_up;
uniform vec3 cam_front;
uniform vec3 cam_right;

in vec3 cameraPos;
in vec3 viewDir;
in vec2 TexCoords;

out vec4 FragColor;

float rs = 2.0 * M;

vec3 starless(vec3 pos, float h2){
    return -3 * h2 * M * pos / pow(dot(pos, pos), 2.5);
}

void main() {
    
    FragColor = vec4(0,0,0,0);

    vec3 dir = normalize(viewDir);
    dir = normalize(dir.x * cam_right + dir.y * cam_up - dir.z * cam_front);
  
    float dotP = dot(normalize(-cameraPos), dir);
    float dist = length(dotP * length(cameraPos) * dir + cameraPos);
    // stop if lightray is pointing directly to black hole
    
    vec3 lightPos = cameraPos;
    vec3 lightVel = dir;
    vec3 lightup = cross(cameraPos, lightVel);
    float h2 = dot(lightup, lightup);

    // simple loop for now
    float step = stepSize;
    for(int i = 0; i < 100; ++i) {

        step = max(0.0001, stepSize * (length(lightPos) - rs));

        vec3 acc = starless(lightPos, h2);
        lightVel += acc * step;
        lightPos += lightVel * step;
        
        if (dot(lightPos, lightVel) > 0 && 
            length(acc*step) <= length(lightVel) * 0.001) {
            break;
        }
         
        if(length(lightPos) <= rs){
            return;
        }

    }

    FragColor.rgb += (1.0-FragColor.a) * texture(cubeMap, lightVel).rgb;
    FragColor.a = 1.0;
}