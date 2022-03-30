in vec3 modelPos;

out vec4 FragColor;

float vecMax (vec3 v) {
   return max(max(v.x, v.y), v.z);
}

vec2 getUV(vec3 v) {
   vec3 absV = abs(v);
   float maxAxis;
   vec2 uv;

   if(absV.x > absV.y && absV.x > absV.z) {
      maxAxis = v.x;
      uv = vec2(v.x > 0 ? v.z : -v.z, v.y);
   } else if(absV.y > absV.x && absV.y > absV.z) {
      maxAxis = v.y;
      uv = vec2(v.x, v.y > 0 ? -v.z : v.z);
   } else {
      maxAxis = v.z;
      uv = vec2(v.z > 0 ? v.x : -v.x, v.y);
   }

   return 0.5 * (uv / maxAxis + 1.0);
}

void main() {  

   vec3 dir = normalize(modelPos);
   float max = vecMax(abs(dir));

   vec2 coord = getUV(modelPos);
   // if(abs(dir.x) == max) {
   //    coord = (vec2(1.0) + dir.yz) * 0.5;
   // } else if (abs(dir.y) == max) {
   //    coord = (vec2(1.0) + dir.xz) * 0.5;
   // }

   ivec2 checker = ivec2(mod(coord * 5.0, vec2(2.0)));
   float tile = checker.x == checker.y ? 1.0 : 0.0;
   // FragColor = vec4(abs(normalize(modelPos))*tile, 1.0);
   FragColor = vec4(coord, 1.0 - 0.8 + 0.8 * tile, 1.0);
   // FragColor = vec4(tile,0,0,1);

}