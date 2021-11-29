/////// simplified version of bruneton's black_hole_shader ///////
/////// https://github.com/ebruneton/black_hole_shader ///////

const float kMu = 4.0 / 27.0;
const float rad = 1.0;
const float pi = 3.14159265359;

const float ADMIN = 3.0;
const float ADMAX = 12.0;
const float I_ADMIN = 1.0 / ADMIN;
const float I_ADMAX = 1.0 / ADMAX;

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

vec2 LookupRayDeflection(sampler2D tex, float e_square, float u,
                               out vec2 deflection_apsis) {

    ivec2 texDim = textureSize(tex, 0);
    float tex_u = GetTextureCoordFromUnitRange(
      GetRayDeflectionTextureUFromEsquare(e_square),
      texDim.x);
    float tex_v = GetTextureCoordFromUnitRange(
      GetRayDeflectionTextureVFromEsquareAndU(e_square, u),
      texDim.y);
    float tex_v_apsis = GetTextureCoordFromUnitRange(1.0, texDim.y);
    deflection_apsis =
      vec2(texture(tex, vec2(tex_u, tex_v_apsis)));
    return texture(tex, vec2(tex_u, tex_v)).xy;
}

float GetRayInverseRadiusTextureUFromEsquare(float e_square) {
    return 1.0 / (1.0 + 6.0 * e_square);
}

float GetPhiUbFromEsquare(float e_square) {
    return (1.0 + e_square) / (1.0 / 3.0 + 2.0 * e_square * sqrt(e_square)) * rad;
}

vec2 LookupRayInverseRadius(sampler2D tex, float e_square, float phi) {
    ivec2 texDim = textureSize(tex, 0);
    float tex_u = GetTextureCoordFromUnitRange(
        GetRayInverseRadiusTextureUFromEsquare(e_square),
        texDim.x);
    float tex_v = GetTextureCoordFromUnitRange(phi / GetPhiUbFromEsquare(e_square),
        texDim.y);
    return texture(tex, vec2(tex_u, tex_v)).xy;
}

float RayTrace(float u, float u_dot, float e_square, float delta, float alpha,
    out float u0, out float phi0, out float u1, out float phi1,
    sampler2D deflection_texture, sampler2D inv_radius_texture){
    
    u0 = -1.0;
    u1 = -1.0;
    if (e_square < kMu && u > 2.0 / 3.0) {
        return -1.0 * rad;
    }
    vec2 deflection_apsis;
    vec2 deflection = LookupRayDeflection(deflection_texture, e_square, u, deflection_apsis);
    float ray_deflection = deflection.x;

    if (u_dot > 0.0) {
        ray_deflection =
            e_square < kMu ? 2.0 * deflection_apsis.x - ray_deflection : -1.0 * rad;
    }

    // Compute the accretion disc intersections.
    float s = sign(u_dot);
    float phi = deflection.x + (s == 1.0 ? pi - delta : delta) + s * alpha;
    float phi_apsis = deflection_apsis.x + pi / 2.0;
    phi0 = mod(phi, pi);
    vec2 ui0 =
      LookupRayInverseRadius(inv_radius_texture, e_square, phi0);
    if (phi0 < phi_apsis) {
        float side = s * (ui0.x - u);
        if (side > 1e-3 || (side > -1e-3 && alpha < delta)) {
          u0 = ui0.x;
          phi0 = alpha + phi - phi0;
        }
    }
    phi = 2.0 * phi_apsis - phi;
    phi1 = mod(phi, pi);
    vec2 ui1 =
      LookupRayInverseRadius(inv_radius_texture, e_square, phi1);
    if (e_square < kMu && s == 1.0 && phi1 < phi_apsis) {
        u1 = ui1.x;
        phi1 = alpha + phi - phi1;
    }

    return ray_deflection;
}

vec3 pixelColor(vec3 dir, vec3 pos, samplerCube cubeMap, 
        sampler2D deflection_texture, sampler2D inv_radius_texture) {
    vec3 e_x_prime = normalize(pos);
    vec3 e_z_prime = normalize(cross(e_x_prime, dir));
    vec3 e_y_prime = normalize(cross(e_z_prime, e_x_prime));
    
    const vec3 e_z = vec3(0.0, 0.0, 1.0);
    vec3 t = normalize(cross(e_z, e_z_prime));
    if (dot(t, e_y_prime) < 0.0) {
        t = -t;
    }
    float alpha = acos(clamp(dot(e_x_prime, t), -1.0, 1.0));
    float delta = acos(clamp(dot(e_x_prime, normalize(dir)), -1.0, 1.0));
    float u = 1.0 / length(pos);
    float u_dot = -u / tan(delta);
    float e_square = u_dot * u_dot + u * u * (1.0 - u);

    float u0, phi0, u1, phi1;
    float deflection = RayTrace(u, u_dot, e_square, delta, alpha,
        u0, phi0, u1, phi1, deflection_texture, inv_radius_texture);

    vec3 color = vec3(0,0,0);

    if (deflection >= 0.0) {
        float delta_prime = delta + max(deflection, 0.0);
        vec3 d_prime = cos(delta_prime) * e_x_prime + sin(delta_prime) * e_y_prime;
        d_prime = vec3(d_prime.x, -d_prime.z, d_prime.y);
        color = texture(cubeMap, d_prime).rgb;
    }

#ifdef DISK
    if (u1 >= 0.0 && u1 < I_ADMIN && u1 > I_ADMAX) {
        bool top_side =
            (mod(abs(phi1 - alpha), 2.0 * pi) < 1e-3) == (e_x_prime.z > 0.0);
        vec3 disc_color = vec3(1, top_side, 0);
        color = color * (0.2) + 0.8 * disc_color.rgb;
    }
    if (u0 >= 0.0 && u0 < I_ADMIN && u0 > I_ADMAX) {
        bool top_side =
            (mod(abs(phi0 - alpha), 2.0 * pi) < 1e-3) == (e_x_prime.z > 0.0);

        vec3 disc_color = vec3(1, top_side, 0);
        color = color * (0.2) + 0.8 * disc_color.rgb;    
    }
#endif
    return color;

}