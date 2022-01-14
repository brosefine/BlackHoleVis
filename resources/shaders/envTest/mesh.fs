
in vec2 uv;

layout(binding = 0) uniform sampler2D mesh_texture;

out vec4 FragColor;

void main() {  
   FragColor = vec4(texture(mesh_texture, uv).xyz, 1);
   //FragColor = vec4(0.2,1,0,1); 
}