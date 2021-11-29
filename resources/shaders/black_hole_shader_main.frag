out vec4 FragColor;

in vec3 cameraPos;
in vec3 viewDir;
in vec2 TexCoords;

layout(binding = 0) uniform samplerCube cubeMap;
layout(binding = 1) uniform sampler2D deflection_texture;
layout(binding = 2) uniform sampler2D inv_radius_texture;

uniform vec3 cam_tau;
uniform vec3 cam_up;
uniform vec3 cam_front;
uniform vec3 cam_right;

uniform bool pinhole = true;

void main()
{    
    vec3 dir = normalize(viewDir);
    if(!pinhole) {

        float phi = (1.0-TexCoords.x) * 2.0 * pi;
        float theta = (1.0-TexCoords.y) * pi;
        dir.x = sin(phi) * sin(theta);
        dir.y = cos(theta);
        dir.z = cos(phi) * sin(theta);

    }
    
    dir = normalize(-cam_tau + dir.x * cam_right + dir.y * cam_up - dir.z * cam_front);

    dir = vec3(dir.x, -dir.z, dir.y);
    vec3 pos = vec3(cameraPos.x, -cameraPos.z, cameraPos.y);

    vec3 color = pixelColor(dir, pos, 
        cubeMap, deflection_texture, inv_radius_texture);

    FragColor = vec4(color, 1.0); 
}