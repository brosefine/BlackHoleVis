
in vec2 uv;

layout(binding = 0) uniform sampler2D mesh_texture;
uniform vec3 mesh_color;
uniform bool use_texture;

out vec4 FragColor;

void main() {  
   if(use_texture)
      FragColor = vec4(texture(mesh_texture, uv).xyz, 1);
   else
      FragColor = vec4(mesh_color, 1);

   //FragColor = vec4(1,0,0,1);

}