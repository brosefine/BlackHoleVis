/////// simplified version of bruneton's black_hole_shader ///////
/////// https://github.com/ebruneton/black_hole_shader ///////

out vec4 FragColor;

in vec3 viewDir;
in vec3 cameraPos;

layout(binding = 0) uniform sampler2D deflection_texture;
layout(binding = 1) uniform samplerCube cubeMap;

uniform vec3 cam_up;
uniform vec3 cam_front;
uniform vec3 cam_right;

const float kMu = 4.0 / 27.0;
const float rad = 1.0;
const float pi = 3.14159265359;

float GetUapsisFromEsquare(float e_square) {
  float x = (2.0 / kMu) * e_square - 1.0;
  return 1.0 / 3.0 + (2.0 / 3.0) * sin(asin(x) * (1.0 / 3.0));
}

float GetRayDeflectionTextureUFromEsquare(float e_square) {
  if (e_square < kMu) {
    return 0.5 - sqrt(-log(1.0 - e_square / kMu) * (1.0 / 50.0));
  } else {
    return 0.5 + sqrt(-log(1.0 - kMu / e_square) * (1.0 / 50.0));
  }
}

float GetRayDeflectionTextureVFromEsquareAndU(float e_square,
                                             float u) {
  if (e_square > kMu) {
    float x = u < 2.0 / 3.0 ? -sqrt(2.0 / 3.0 - u) : sqrt(u - 2.0 / 3.0);
    return (sqrt(2.0 / 3.0) + x) / (sqrt(2.0 / 3.0) + sqrt(1.0 / 3.0));
  } else {
    return 1.0 - sqrt(max(1.0 - u / GetUapsisFromEsquare(e_square), 0.0));
  }
}


float GetTextureCoordFromUnitRange(float x, int texture_size) {
  return 0.5 / float(texture_size) + x * (1.0 - 1.0 / float(texture_size));
}

vec2 LookupRayDeflection(float e_square, float u,
                               out vec2 deflection_apsis) {

    ivec2 texDim = textureSize(deflection_texture, 0);
    float tex_u = GetTextureCoordFromUnitRange(
      GetRayDeflectionTextureUFromEsquare(e_square),
      texDim.x);
    float tex_v = GetTextureCoordFromUnitRange(
      GetRayDeflectionTextureVFromEsquareAndU(e_square, u),
      texDim.y);
    float tex_v_apsis = GetTextureCoordFromUnitRange(1.0, texDim.y);
    deflection_apsis =
      vec2(texture(deflection_texture, vec2(tex_u, tex_v_apsis)));
    return vec2(texture(deflection_texture, vec2(tex_u, tex_v)));
}

float RayTrace(float u, float u_dot, float e_square, float delta){
    if (e_square < kMu && u > 2.0 / 3.0) {
        return -1.0 * rad;
    }
    vec2 deflection_apsis;
    vec2 deflection = LookupRayDeflection(e_square, u, deflection_apsis);
    float ray_deflection = deflection.x;

    if (u_dot > 0.0) {
        ray_deflection =
            e_square < kMu ? 2.0 * deflection_apsis.x - ray_deflection : -1.0 * rad;
    }

    return ray_deflection;
}


void main()
{    
    
    vec3 q = normalize(viewDir);
    vec3 up = cross(cam_right, cam_front);
    vec3 dir = q.x * cam_right + q.y * up - q.z * cam_front;

    vec3 e_x_prime = normalize(cameraPos);
    vec3 e_z_prime = normalize(cross(e_x_prime, dir));
    vec3 e_y_prime = normalize(cross(e_z_prime, e_x_prime));
    
    float delta = acos(clamp(dot(e_x_prime, normalize(dir)), -1.0, 1.0));
    float u = 1.0 / length(cameraPos);
    float u_dot = -u / tan(delta);
    float e_square = u_dot * u_dot + u * u * (1.0 - u);

    float deflection = RayTrace(u, u_dot, e_square, delta);

    vec3 color = vec3(0,0,0);

    if (deflection >= 0.0) {
        float delta_prime = delta + max(deflection, 0.0);
        vec3 d_prime = cos(delta_prime) * e_x_prime + sin(delta_prime) * e_y_prime;
        color = texture(cubeMap, d_prime).rgb;
        //color = vec3(1, deflection, 0);
    } else {
        //color = vec3(0,0,1);
    }

    FragColor = vec4(color, 1.0); 
    //FragColor = vec4(1, e_square/kMu, 0, 1.0); 
    //FragColor = vec4(1, 0, u/(2.0/3.0), 1.0); 
}