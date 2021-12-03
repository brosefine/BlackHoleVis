out vec4 FragColor;

in vec3 cameraPos;
in vec3 viewDir;
in vec2 TexCoords;

layout(binding = 0) uniform samplerCube cubeMap;
layout(binding = 1) uniform sampler2D deflection_texture;
layout(binding = 2) uniform sampler2D inv_radius_texture;
layout(binding = 3) uniform sampler2D disk_texture;
layout(binding = 4) uniform sampler2D black_body_texture;
layout(binding = 5) uniform sampler2D noise_texture;

uniform vec3 cam_tau;
uniform vec3 cam_up;
uniform vec3 cam_front;
uniform vec3 cam_right;
uniform vec4 ks;
uniform float dt;

uniform bool pinhole = true;

void main()
{    
    vec3 dir = normalize(viewDir);
    if(!pinhole) {

        float phi = (1.0 - TexCoords.x) * 2.0 * pi;
        float theta = (1.0 - TexCoords.y) * pi;
        dir.x = sin(phi) * sin(theta);
        dir.y = cos(theta);
        dir.z = cos(phi) * sin(theta);

    }
    
    dir = normalize(-cam_tau + dir.x * cam_right + dir.y * cam_up - dir.z * cam_front);
    dir = vec3(dir.x, -dir.z, dir.y);
    vec4 k = vec4(ks.x, -ks.w, ks.y, ks.z);
    vec3 pos = vec3(cameraPos.x, -cameraPos.z, cameraPos.y);
    vec3 tau = vec3(cam_tau.x, -cam_tau.z, cam_tau.y);

    vec3 color = pixelColor(dir, pos, tau, k, dt*10,
        cubeMap, deflection_texture, 
        inv_radius_texture, disk_texture, 
        black_body_texture, noise_texture);

    FragColor = vec4(color, 1.0); 
    //FragColor = vec4((texture(noise_texture, TexCoords).rrr),1);
}