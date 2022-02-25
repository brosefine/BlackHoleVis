
in vec3 viewDir;

layout(binding = 0) uniform samplerCube sky;

uniform vec3 cam_up;
uniform vec3 cam_front;
uniform vec3 cam_right;

out vec4 FragColor;

void main() {  
   vec3 dir = viewDir.x * cam_right + viewDir.y * cam_up + viewDir.z * cam_front;
   FragColor = vec4(texture(sky, dir).xyz, 1);
   //FragColor = vec4(1,0,0,1);
}