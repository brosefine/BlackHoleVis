
in vec3 viewDir;

layout(binding = 0) uniform samplerCube sky;

out vec4 FragColor;

void main() {  
   FragColor = vec4(texture(sky, viewDir).xyz, 1);
}