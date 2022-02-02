/**
 simplified version of bruneton's black_hole_shader 
 https://github.com/ebruneton/black_hole_shader 
 * Copyright (c) 2020 Eric Bruneton
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifdef DISC 
layout (std140) uniform accDisk
{
    vec4 discParams; // density, opacity, temperature, numparticles
    vec4 discSize;   // rmin, rmax, irmin, irmax
};
uniform float max_brightness;
#endif

out vec4 FragColor;

in vec3 cameraPos;
in vec3 viewDir;
in vec2 TexCoords;

layout(binding = 0) uniform samplerCube cubeMap;
layout(binding = 1) uniform sampler2D deflection_texture;
layout(binding = 2) uniform sampler2D inv_radius_texture;
layout(binding = 3) uniform sampler2D jet_texture;
layout(binding = 4) uniform sampler2D black_body_texture;
layout(binding = 5) uniform sampler3D doppler_texture;
layout(binding = 6) uniform samplerCube star_texture;
layout(binding = 7) uniform samplerCube star_texture2;
layout(binding = 8) uniform samplerCube star_textureFull;
layout(binding = 9) uniform sampler2D noise_texture;

uniform float scale = 1.0;
uniform vec3 cam_tau;
uniform vec3 cam_up;
uniform vec3 cam_front;
uniform vec3 cam_right;
uniform vec4 ks;
uniform float dt;

uniform float jet_angle;
uniform vec4 jet_size; // x = min, y = max, z = umin, w = umax

uniform bool pinhole = true;
uniform bool gaiaMap;

const float MAX_FOOTPRINT_LOD = 6.0;
const int STARS_CUBE_MAP_SIZE = 2048;
const float MAX_FOOTPRINT_SIZE = 4.0;

const float kMu = 4.0 / 27.0;
const float pi = 3.14159265359;
// temperature is highest at radius of ~ 4.08;
const float r_max_temp = 49.0 / 12.0;
const float max_temp = 
    pow((1.0 - sqrt(3.0 / r_max_temp)) / (r_max_temp * r_max_temp * r_max_temp), 0.25);

#ifdef DISC 
const vec4[10] particles = vec4[10] (
vec4(0.33333334,0.31661114,6.1977997,0.07803755),
vec4(0.25,0.2162681,2.9107437,0.27628574),
vec4(0.2,0.18788706,4.4959383,0.32610515),
vec4(0.16666667,0.14545912,5.5849767,0.36773542),
vec4(0.14285715,0.14210331,0.4037387,0.38162342),
vec4(0.125,0.115369216,4.338544,0.40328234),
vec4(0.11111111,0.105860256,2.4303732,0.4142117),
vec4(0.1,0.08681646,5.188272,0.42785874),
vec4(0.09090909,0.083897986,0.008218025,0.43319297),
vec4(0.083333336,0.0729598,2.4058197,0.4412647)
);
#endif // DISC 


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
    return texture(deflection_texture, vec2(tex_u, tex_v)).xy;
}

float GetRayInverseRadiusTextureUFromEsquare(float e_square) {
    return 1.0 / (1.0 + 6.0 * e_square);
}

float GetPhiUbFromEsquare(float e_square) {
    return (1.0 + e_square) / (1.0 / 3.0 + 2.0 * e_square * sqrt(e_square));
}

vec2 LookupRayInverseRadius(float e_square, float phi) {
    ivec2 texDim = textureSize(inv_radius_texture, 0);
    float tex_u = GetTextureCoordFromUnitRange(
        GetRayInverseRadiusTextureUFromEsquare(e_square),
        texDim.x);
    float tex_v = GetTextureCoordFromUnitRange(phi / GetPhiUbFromEsquare(e_square),
        texDim.y);
    vec2 result = texture(inv_radius_texture, vec2(tex_u, tex_v)).xy;
    //if(e_square < 1e-3) result.x = sqrt(e_square) * sin(phi);
    return result * sqrt(e_square);
}

vec3 Doppler(vec3 color, float doppler_factor) {
    float sum = color.x + color.y + color.z;
    if (sum == 0.0) return vec3(0);
    vec3 dopplerTexCoord;
    dopplerTexCoord.x = color.x / sum;
    dopplerTexCoord.y = 2.0 * color.y / sum;
    dopplerTexCoord.z =
        (1.0 / 3.0) * atan((1.0 / 0.21) * log(doppler_factor)) + 0.5;
    return sum * texture(doppler_texture, dopplerTexCoord).rgb;
}

// s = sign(u_dot)
void Intersect(float s, float u, float e_square, float deflection, float deflection_apsis, float delta, float alpha,
    out float phi0, out float phi1, out float u0, out float u1) {

    // if ray direction is pointing away from BH, pi = phi + delta + deflection
    float phi = deflection + (s > 0 ? pi - delta : delta) + s * alpha;
    float phi_apsis = deflection_apsis + pi / 2.0;
    // first intersection
    phi0 = mod(phi, pi);
    if (phi0 < phi_apsis) {
        vec2 ui0 =
          LookupRayInverseRadius(e_square, phi0);
        // s > 0 if ray looks towards bh
        // ui0.x > u if hit is closer to black hole
        float side = s * (ui0.x - u);
        // if light ray direction + hit point match, or if udot = 0
        if ((side > 1e-3 ) || (side > -1e-3 && alpha < delta)) {
            u0 = ui0.x;
            phi0 = alpha + phi - phi0;
        }
    }
    // second intersection
    phi = 2.0 * phi_apsis - phi;
    phi1 = mod(phi, pi);
    if (e_square < kMu && s == 1.0 && phi1 < phi_apsis) {
        vec2 ui1 =
          LookupRayInverseRadius(e_square, phi1);
        u1 = ui1.x;
        phi1 = alpha + phi - phi1;
    }
}

#ifdef JET
float RayTrace(float u, float u_dot, float e_square, float delta, 
    float disk_alpha, float front_jet_alpha, float back_jet_alpha, bool jet_hit,
    out float disk_u0, out float disk_phi0,
    out float disk_u1, out float disk_phi1,
    out float front_jet_u0, out float front_jet_phi0,  
    out float front_jet_u1, out float front_jet_phi1,
    out float back_jet_u0, out float back_jet_phi0,  
    out float back_jet_u1, out float back_jet_phi1){
#else
float RayTrace(float u, float u_dot, float e_square, float delta, 
    float disk_alpha, 
    out float disk_u0, out float disk_phi0,
    out float disk_u1, out float disk_phi1){
#endif // JET
    
    disk_u0 = -1.0;
    disk_u1 = -1.0;

    #ifdef JET
    front_jet_u0 = -1.0;
    front_jet_u1 = -1.0;
    back_jet_u0 = -1.0;
    back_jet_u1 = -1.0;
    #endif // JET

    if (e_square < kMu && u > 2.0 / 3.0) {
        return -1.0;
    }
    vec2 deflection_apsis;
    vec2 deflection = LookupRayDeflection(e_square, u, deflection_apsis);
    float ray_deflection = deflection.x;

    if (u_dot > 0.0) {
        // delta_inf - delta
        ray_deflection =
            e_square < kMu ? 2.0 * deflection_apsis.x - ray_deflection : -1.0;
    }

    // Compute the accretion disc intersections.
    float s = sign(u_dot);
    Intersect(s, u, e_square, deflection.x, deflection_apsis.x, delta, disk_alpha,
        disk_phi0, disk_phi1, disk_u0, disk_u1);
    
#ifdef JET
    if(!jet_hit) return ray_deflection;

    Intersect(s, u, e_square, deflection.x, deflection_apsis.x, delta, front_jet_alpha,
        front_jet_phi0, front_jet_phi1, front_jet_u0, front_jet_u1);

    Intersect(s, u, e_square, deflection.x, deflection_apsis.x, delta, back_jet_alpha,
        back_jet_phi0, back_jet_phi1, back_jet_u0, back_jet_u1);

    
#endif // JET
    return ray_deflection;
}

#ifdef DISC
vec4 DiscColor(vec2 intersect, float timeDelta,
    float doppler) {
    float r_intersect = length(intersect);
    // scale interesction point to dimension of disc particles (3-13)
    float p_r = (r_intersect - discSize.x) / (discSize.y - discSize.x) * 10 + 3.0;
    float p_phi = atan(intersect.y, intersect.x);

    float density = 0.3;
    for(int i = 0; i < discParams.w; ++i) {
        vec4 params = particles[i];
        float u1 = params.x;
        float u2 = params.y;
        float u_mid = (u1 + u2) * 0.5;
        // scale u_mid to dimension of user-defined accretion disc
        u_mid = 1.0/((1.0/u_mid - 3.0)*(discSize.y-discSize.x) / 10 + discSize.x);
        float phi0 = params.z;
        float dRdPhi = params.w;
        float dPhidT = u_mid * sqrt(0.5*u_mid);
        float phi = dPhidT * timeDelta + phi0;

        float closestPoint = mod(p_phi - phi, 2.0 * pi);
        float u_particle = u1 + (u2 - u1) * pow(sin(dRdPhi * (closestPoint + phi)), 2.0); 
        float r_particle = 1.0 / u_particle;
        vec2 d = vec2(closestPoint-pi, r_particle-p_r) * vec2(1.0 / pi, 0.23);
        vec2 noise_uv = d * vec2(p_r/12.0, 1.0);
        float noise = 2.0 * (texture(noise_texture, noise_uv).r - 0.5) + 1.0;
        density += smoothstep(1.0, 0.0, length(d))*noise;
    }

    
    float tempScale = (1.0 / max_temp) *
        pow((1.0 - sqrt(3.0 / r_intersect)) / (r_intersect * r_intersect * r_intersect), 0.25);
    float temp = doppler * tempScale * discParams.z;
    float temp_coord = (1.0/6.0) * log(temp/100.0);
    vec3 color = texture(black_body_texture, vec2(temp_coord, 0.5)).rgb;
    if(length(color) > 0){
        float max_component = max(color.r, max(color.g, color.b));
        color = color / max_component * min(max_brightness, max_component);
    } 
    color *= max(density, 0.0);

    float alpha = smoothstep(3.0, 3.0 * 1.2, p_r) *
        smoothstep(12.0, 12.0 / 1.2, p_r);
    
    /*
    // rotation speed depends on distance to BH
    vec3 color = texture(tex, vec2((p_phi/(2*pi)), (p_r - discSize.x)/(discSize.y-discSize.x))).rrr;
    //return vec4(timeDelta, top, 0, alpha);
    */
    //vec3 color = vec3(max(0.0,density)) * discParams.x;
    return vec4(color*discParams.x, alpha*discParams.y);
}
#endif

#ifdef STARS
vec3 StarColor(vec3 dir, float lensing_amplification_factor,
                      float min_lod) {

  // Compute the partial derivatives of dir (continuous across cube edges).
  vec3 dx_dir = dFdx(dir);
  vec3 dy_dir = dFdy(dir);

  // Swap the coordinates depending on the cube face, to always get the maximum
  // absolute value of the 'dir' components in the z coordinate.
  vec3 abs_dir = abs(dir);
  float max_abs_dir_comp = max(abs_dir.x, max(abs_dir.y, abs_dir.z));
  if (max_abs_dir_comp == abs_dir.x) {
    dir = dir.zyx;
    dx_dir = dx_dir.zyx;
    dy_dir = dy_dir.zyx;
  } else if (max_abs_dir_comp == abs_dir.y) {
    dir = dir.xzy;
    dx_dir = dx_dir.xzy;
    dy_dir = dy_dir.xzy;
  }

  // Compute the cube face texture coordinates uv and their derivatives dx_uv
  // and dy_uv (using an analytic formula instead of dFdx and dFdy, to avoid
  // discontinuities at cube edges - uv is not continuous here).
  float inv_dir_z = 1.0 / dir.z;
  vec2 uv = dir.xy * inv_dir_z;
  vec2 dx_uv = (dx_dir.xy - uv * dx_dir.z) * inv_dir_z;
  vec2 dy_uv = (dy_dir.xy - uv * dy_dir.z) * inv_dir_z;

  // Compute the LOD level to use to fetch the stars in the footprint of 'dir'.
  vec2 d_uv = max(abs(dx_uv + dy_uv), abs(dx_uv - dy_uv));
  vec2 fwidth = (0.5 * STARS_CUBE_MAP_SIZE / MAX_FOOTPRINT_SIZE) * d_uv;
  float lod_fl = max((max(log2(fwidth.x), log2(fwidth.y))), min_lod);
  float lod = ceil(lod_fl);
  float lod_width = (0.5 * STARS_CUBE_MAP_SIZE) / pow(2.0, lod);
  if (lod > MAX_FOOTPRINT_LOD) {
   //return vec3(1,0,0);
    return texture(star_texture2, dir).rgb * lensing_amplification_factor;
  }
  // Fetch, filter and accumulate the colors of the stars in the texels in the
  // footprint of 'dir' at 'lod'.
  mat2 to_screen_pixel_coords = inverse(mat2(dx_uv, dy_uv));
  ivec2 ij0 = ivec2(floor((uv - d_uv) * lod_width));
  ivec2 ij1 = ivec2(floor((uv + d_uv) * lod_width));
  vec3 color_sum = vec3(0.0);
  for (int j = ij0.y; j <= ij1.y; ++j) {
    for (int i = ij0.x; i <= ij1.x; ++i) {
      vec2 texel_uv = (vec2(i, j) + vec2(0.5)) / lod_width;
      vec3 texel_dir = vec3(texel_uv * dir.z, dir.z);
      if (max_abs_dir_comp == abs_dir.x) {
        texel_dir = texel_dir.zyx;
      } else if (max_abs_dir_comp == abs_dir.y) {
        texel_dir = texel_dir.xzy;
      }
      vec3 color = textureLod(star_texture, texel_dir, lod).rgb;   
      ivec2 bits = floatBitsToInt(color.rb);
      vec2 delta_uv = vec2((bits >> 8) % 257) / 257.0 - vec2(0.5);
      vec2 star_uv = uv - texel_uv + delta_uv / lod_width;
      vec2 star_pixel_coords = to_screen_pixel_coords * star_uv;
      vec2 overlap = max(vec2(1.0) - abs(star_pixel_coords), 0.0);
      color_sum += color * overlap.x * overlap.y;
    }
  }
  return color_sum * lensing_amplification_factor;
}
#endif

#ifdef JET
vec4 JetColor(vec3 intersect, float timeDelta) {

    float r_intersect = length(intersect);
    float texV = 1.0 - (r_intersect - jet_size.x)/(jet_size.y-jet_size.x);
    float texU = atan(intersect.y, intersect.x) + pi;
    float texU1 = texU - timeDelta/5.0;
    float texU2 = texU + timeDelta/4.0;
    texU1 = mod(texU1, 2.0*pi) / (2.0 * pi);
    texU2 = 1.0 - mod(texU2, 2.0*pi) / (2.0 * pi);
    vec3 color1 = vec3(0.6, 0.8, 1.0) * max_brightness; 
    vec3 color2 = vec3(0.8, 1, 0.6)* max_brightness; 
    float alpha1 = texture(jet_texture, vec2(texU1, texV)).r * texture(noise_texture, vec2(r_intersect/2.0, (texU - timeDelta/4.0)/2.0)).r; 
    float alpha2 = texture(jet_texture, vec2(texU2, texV)).r * texture(noise_texture, vec2(r_intersect/3.0, (texU + timeDelta/4.0)/2.0)).r;
    float alpha =  smoothstep(jet_size.x, jet_size.x * 2.5, r_intersect) 
        *  smoothstep(jet_size.y, jet_size.y / 2.5, r_intersect);

    return vec4(color1/alpha2 + color2/alpha1, alpha*alpha1*alpha2);
}
#endif

vec3 pixelColor(vec3 dir, vec3 pos, vec3 etau, vec4 ks, float dt) {
    vec3 e_x_prime = normalize(pos);
    vec3 e_z_prime = normalize(cross(e_x_prime, dir));
    vec3 e_y_prime = normalize(cross(e_z_prime, e_x_prime));
    
    const vec3 e_z = vec3(0.0, 0.0, 1.0);
    vec3 t = normalize(cross(e_z, e_z_prime));
    if (dot(t, e_y_prime) < 0.0) {
        t = -t;
    }


    float disk_alpha = acos(clamp(dot(e_x_prime, t), -1.0, 1.0));
    float delta = acos(clamp(dot(e_x_prime, normalize(dir)), -1.0, 1.0));
    float u = 1.0 / length(pos);
    float u_dot = -u / tan(delta);
    float e_square = u_dot * u_dot + u * u * (1.0 - u);
    float e = -sqrt(e_square);

    float disk_u0, disk_phi0, disk_u1, disk_phi1;
#ifndef JET
    float deflection = RayTrace(u, u_dot, e_square, delta, disk_alpha,
            disk_u0, disk_phi0, disk_u1, disk_phi1);
#else
    float front_jet_alpha, back_jet_alpha;
    bool jet_hit = false;

    vec3 z = vec3(0.0, 0.0, 1.0);
    if ( e_y_prime.z < 0.0 ) z = -z;
    vec3 z_prime = normalize(z - dot(z, e_z_prime) * e_z_prime);
    float a = acos(dot(z_prime, e_x_prime));
    float b = acos(dot(z_prime, z));
    jet_hit = b <= jet_angle;

    if(jet_hit) {

        float angle_dist = 1.0 - (jet_angle - b)/jet_angle;
        float f = sqrt(1.0 - (angle_dist*angle_dist));
        front_jet_alpha = a - jet_angle * f;
        back_jet_alpha = a + jet_angle * f;
    } 
    
    float front_jet_u0, front_jet_phi0, front_jet_u1, front_jet_phi1;
    float back_jet_u0, back_jet_phi0, back_jet_u1, back_jet_phi1;

    float deflection = RayTrace(u, u_dot, e_square, delta, disk_alpha, front_jet_alpha, back_jet_alpha, jet_hit,
        disk_u0, disk_phi0, disk_u1, disk_phi1, 
        front_jet_u0, front_jet_phi0, front_jet_u1, front_jet_phi1,
        back_jet_u0, back_jet_phi0, back_jet_u1, back_jet_phi1);
#endif //JET

    vec4 l = vec4(e / (1.0 - u), -u_dot, 0.0, u * u);
    float g_k_l_receiver = ks.x * l.x * (1.0 - u) - ks.y * l.y / (1.0 - u) -
                         u * dot(etau, e_y_prime) * l.w / (u * u);
    vec3 color = vec3(0,0,0);

    if (deflection >= 0.0) {
        
        float delta_prime = delta + max(deflection, 0.0);
        vec3 d_prime = cos(delta_prime) * e_x_prime + sin(delta_prime) * e_y_prime;

        if(!gaiaMap) d_prime = vec3(d_prime.x, d_prime.z, -d_prime.y);
        color += texture(cubeMap, d_prime).rgb;
        if(gaiaMap) color *= 6.78494e-5;
     
        #ifdef STARS
        // The solid angle (times 4pi) of the pixel.
        float omega = length(cross(dFdx(dir), dFdy(dir)));
        // The solid angle (times 4pi) of the deflected light beam.
        float omega_prime = length(cross(dFdx(d_prime), dFdy(d_prime)));

        float lensing_amplification_factor = omega / omega_prime;
        // Clamp the result (otherwise potentially infinite).
        lensing_amplification_factor = min(lensing_amplification_factor, 1e6);

        float pixel_area = max(omega * (1024.0 * 1024.0), 1.0);

        //color += texture(star_textureFull, d_prime).rgb * lensing_amplification_factor/pixel_area;
        color += StarColor(d_prime, lensing_amplification_factor/pixel_area, 0.0);
        #endif

        #ifdef DOPPLER
        float g_k_l_source = e;
        float doppler_factor = g_k_l_receiver / g_k_l_source;
        color = Doppler(color, doppler_factor);
        #endif
    }

#ifdef DISC
    if (disk_u1 >= 0.0 && disk_u1 < discSize.z && disk_u1 > discSize.w) {
        #ifdef DOPPLER
        float g_k_l_source = e * sqrt(2.0 / (2.0 - 3.0 * disk_u1)) -
                         disk_u1 * sqrt(disk_u1 / (2.0 - 3.0 * disk_u1)) * dot(e_z, e_z_prime);
        float doppler_factor = g_k_l_receiver/g_k_l_source;
        #else
        float doppler_factor = 1.0;
        #endif
        vec3 intersect = (e_x_prime * cos(disk_phi1) + e_y_prime * sin(disk_phi1))/disk_u1;
        vec4 disc_color = DiscColor(intersect.xy, dt, doppler_factor);
        color = color * (1.0-disc_color.a) + disc_color.a * disc_color.rgb;
        //color = 0.5 * color + 0.5 * vec3(1,0,0);
    }
#endif // DISC

#ifdef JET
    if(jet_hit) {
        float jet_fade = smoothstep(jet_angle, jet_angle*0.1, b);
        if ( back_jet_u1 >= 0.0 && back_jet_u1 < jet_size.z && back_jet_u1 > jet_size.w) {

            float back_jet_r1 = 1.0 / back_jet_u1;
            vec3 intersect = (e_x_prime * cos(back_jet_phi1) + e_y_prime * sin(back_jet_phi1))/back_jet_u1;
            vec4 jet_color = JetColor(intersect, dt);//vec4(0.0,1.0,0.0,1);
            jet_color.a *= jet_fade;
            color = color * (1.0-jet_color.a) + jet_color.a * jet_color.rgb;
        }
        
        if ( back_jet_u0 >= 0.0 && back_jet_u0 < jet_size.z && back_jet_u0 > jet_size.w) {

            float back_jet_r0 = 1.0 / back_jet_u0;
            vec3 intersect = (e_x_prime * cos(back_jet_phi0) + e_y_prime * sin(back_jet_phi0))/back_jet_u0;
            vec4 jet_color = JetColor(intersect, dt);//vec4(0.0,1.0,0.0,1);
            jet_color.a *= jet_fade;
            color = color * (1.0-jet_color.a) + jet_color.a * jet_color.rgb;
        }
        
        
        if ( front_jet_u1 >= 0.0 && front_jet_u1 < jet_size.z && front_jet_u1 > jet_size.w) {

            float front_jet_r1 = 1.0 / front_jet_u1;
            vec3 intersect = (e_x_prime * cos(front_jet_phi1) + e_y_prime * sin(front_jet_phi1))/front_jet_u1;
            vec4 jet_color = JetColor(intersect, dt);//vec4(0.0,0.0,1.0,1);
            jet_color.a *= jet_fade;
            color = color * (1.0-jet_color.a) + jet_color.a * jet_color.rgb;
        }
        
        if ( front_jet_u0 >= 0.0 && front_jet_u0 < jet_size.z && front_jet_u0 > jet_size.w) {

            float front_jet_r0 = 1.0 / front_jet_u0;
            vec3 intersect = (e_x_prime * cos(front_jet_phi0) + e_y_prime * sin(front_jet_phi0))/front_jet_u0;
            vec4 jet_color = JetColor(intersect, dt);//vec4(0.0,0.0,1.0,1);
            jet_color.a *= jet_fade;
            color = color * (1.0-jet_color.a) + jet_color.a * jet_color.rgb;
        }

    }

#endif //JET

#ifdef DISC
    if (disk_u0 >= 0.0 && disk_u0 < discSize.z && disk_u0 > discSize.w) {
        #ifdef DOPPLER
        float g_k_l_source = e * sqrt(2.0 / (2.0 - 3.0 * disk_u0)) -
                         disk_u0 * sqrt(disk_u0 / (2.0 - 3.0 * disk_u0)) * dot(e_z, e_z_prime);
        float doppler_factor = g_k_l_receiver/g_k_l_source;
        #else
        float doppler_factor = 1.0;
        #endif
        vec3 intersect = (e_x_prime * cos(disk_phi0) + e_y_prime * sin(disk_phi0))/disk_u0;
        vec4 disc_color = DiscColor(intersect.xy, dt, doppler_factor);
        color = color * (1.0-disc_color.a) + disc_color.a * disc_color.rgb;    
        //color = 0.5 * color + 0.5 * vec3(0,1,0);
    }
#endif // DISC

    return color;

}

void main()
{    
    #ifdef PINHOLE
    vec3 dir = normalize(viewDir);
    #else
    float phi = (1.0 - TexCoords.x) * 2.0 * pi;
    float theta = (1.0 - TexCoords.y) * pi;
    vec3 dir;
    dir.x = sin(phi) * sin(theta);
    dir.y = cos(theta);
    dir.z = cos(phi) * sin(theta);
    #endif    

    dir = normalize(-cam_tau + dir.x * cam_right + dir.y * cam_up - dir.z * cam_front);
    vec3 color;
    if(scale <= 0.0) {
        color = texture(cubeMap, dir).rgb;
    } else {


        dir = vec3(dir.x, -dir.z, dir.y);
        vec4 k = vec4(ks.x, -ks.w, ks.y, ks.z);
        vec3 pos = vec3(cameraPos.x, -cameraPos.z, cameraPos.y) / scale;
        vec3 tau = vec3(cam_tau.x, -cam_tau.z, cam_tau.y);

        color = pixelColor(dir, pos, tau, k, dt*10);
    }

    FragColor = vec4(color, 1.0); 
    //FragColor = vec4(texture(jet_texture, TexCoords).rgb, 1);

}