#version 330 core
out vec4 FragColor;

void main() {   
   float x = gl_FragCoord.x/800;
   float y = gl_FragCoord.y/600;
   float dist = abs((x-0.5)*(x-0.5)+(y-0.5)*(y-0.5));
   FragColor = vec4(x, y, (1-x)*(1-y), 1);
   FragColor.rgb *= dist;
}