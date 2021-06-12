#version 330 core

uniform vec3 cameraPos;
uniform vec3 blackHolePos;

in vec3 worldPos;
out vec4 FragColor;

void main() {   
    
    vec3 viewDir = normalize(worldPos - cameraPos);
    vec3 blackHoleDir = normalize(blackHolePos - cameraPos);

    FragColor = vec4(worldPos.xyz*(1-dot(viewDir, blackHoleDir)),1);

}