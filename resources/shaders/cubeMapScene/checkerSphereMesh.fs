#define M_PI 3.14159265

in vec3 modelPos;

layout(binding = 0) uniform sampler2D mesh_texture;

uniform vec3 mesh_color;

out vec4 FragColor;

void main() {  

   float phi = max(0,(atan(modelPos.x, modelPos.z) + M_PI) / (2*M_PI));
   float theta = max(0,(atan(length(modelPos.xz), (modelPos.y)) + M_PI) / (2*M_PI));
   float phiWeight = mod(int(18*phi),2);
   float thetaWeight = mod(int(18*theta),2);
   FragColor = vec4(mesh_color * vec3(phi, vec2(1) * mod(thetaWeight + phiWeight,2)), 1);

}