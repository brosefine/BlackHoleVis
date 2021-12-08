out vec4 FragColor;

uniform vec3 texDims;

void main() {  
   FragColor = vec4(0,0,0,1); 
   float x = gl_FragCoord.x/texDims.x;
   float y = gl_FragCoord.y/texDims.y;
   float dist = abs((x-0.5)*(x-0.5)+(y-0.5)*(y-0.5));
   if(dist < 0.2*0.2)
      FragColor = vec4(1,1,1,1);
}