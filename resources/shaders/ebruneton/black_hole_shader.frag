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
    vec4 particles[20]; // todo: adaptive numer of particles
};
#endif

out vec4 FragColor;

in vec3 cameraPos;
in vec3 viewDir;
in vec2 TexCoords;

layout(binding = 0) uniform samplerCube cubeMap;
layout(binding = 1) uniform sampler2D deflection_texture;
layout(binding = 2) uniform sampler2D inv_radius_texture;
layout(binding = 3) uniform sampler2D disk_texture;
layout(binding = 4) uniform sampler2D black_body_texture;
layout(binding = 5) uniform sampler2D noise_texture;

uniform vec3 cam_tau;
uniform vec3 cam_up;
uniform vec3 cam_front;
uniform vec3 cam_right;
uniform vec4 ks;
uniform float dt;

uniform bool pinhole = true;

const float kMu = 4.0 / 27.0;
const float rad = 1.0;
const float pi = 3.14159265359;
// temperature is highest at radius of ~ 4.08;
const float r_max_temp = 49.0 / 12.0;
const float max_temp = 
    pow((1.0 - sqrt(3.0 / r_max_temp)) / (r_max_temp * r_max_temp * r_max_temp), 0.25);

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
    return (1.0 + e_square) / (1.0 / 3.0 + 2.0 * e_square * sqrt(e_square)) * rad;
}

vec2 LookupRayInverseRadius(float e_square, float phi) {
    ivec2 texDim = textureSize(inv_radius_texture, 0);
    float tex_u = GetTextureCoordFromUnitRange(
        GetRayInverseRadiusTextureUFromEsquare(e_square),
        texDim.x);
    float tex_v = GetTextureCoordFromUnitRange(phi / GetPhiUbFromEsquare(e_square),
        texDim.y);
    return texture(inv_radius_texture, vec2(tex_u, tex_v)).xy;
}

float RayTrace(float u, float u_dot, float e_square, float delta, float alpha,
    out float u0, out float phi0, out float t0, 
    out float u1, out float phi1, out float t1){
    
    u0 = -1.0;
    u1 = -1.0;
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

    // Compute the accretion disc intersections.
    float s = sign(u_dot);
    // if ray direction is pointing away from BH, pi = phi + delta + deflection
    float phi = deflection.x + (s == 1.0 ? pi - delta : delta) + s * alpha;
    float phi_apsis = deflection_apsis.x + pi / 2.0;
    // first intersection
    phi0 = mod(phi, pi);
    if (phi0 < phi_apsis) {
        vec2 ui0 =
          LookupRayInverseRadius(e_square, phi0);
        float side = s * (ui0.x - u);
        if (side > 1e-3 || (side > -1e-3 && alpha < delta)) {
            u0 = ui0.x;
            phi0 = alpha + phi - phi0;
            t0 = s * (ui0.y - deflection.y);
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
        t1 = 2.0 * deflection_apsis.y - ui1.y - deflection.y;
    }

    return ray_deflection;
}

#ifdef DISC
vec4 DiscColor(vec2 intersect, float timeDelta, bool top, 
    float doppler) {
    float p_r = length(intersect);
    float p_phi = atan(intersect.y, intersect.x);

    float density = 0.3;
    for(int i = 0; i < discParams.w; ++i) {
        vec4 params = particles[i];
        float u1 = params.x;
        float u2 = params.y;
        float u_mid = (u1 + u1) * 0.5;
        float phi0 = params.z;
        float dRdPhi = params.w;
        float dPhidT = u_mid * sqrt(0.5*u_mid);
        float phi = dPhidT * timeDelta + phi0;

        float closestPoint = mod(p_phi - phi, 2.0 * pi);
        float u_particle = u1 + (u2 - u1) * pow(sin(dRdPhi * (closestPoint + phi)), 2.0); 
        float r_particle = 1.0 / u_particle;
        vec2 d = vec2(closestPoint-pi, r_particle-p_r) * vec2(1.0 / pi, 0.23);
        vec2 noise_uv = d * vec2(p_r/discSize.y, 1.0);
        float noise = 2.0 * (texture(noise_texture, noise_uv).r - 0.5) + 1.0;
        density += smoothstep(1.0, 0.0, length(d))*noise;
    }

    
    float tempScale = (1.0 / max_temp) *
      pow((1.0 - sqrt(3.0 / p_r)) / (p_r * p_r * p_r), 0.25);
    float temp = doppler * tempScale * discParams.z;
    float temp_coord = (1.0/6.0) * log(temp/100.0);
    vec3 color = texture(black_body_texture, vec2(temp_coord, 0.5)).rgb;
    if(length(color) > 0)color = normalize(color) * min(100000, length(color));
    color *= max(density, 0.0);

    float alpha = smoothstep(discSize.x, discSize.x * 1.2, p_r) *
      smoothstep(discSize.y, discSize.y / 1.2, p_r);
    
    /*
    // rotation speed depends on distance to BH
    vec3 color = texture(tex, vec2((p_phi/(2*pi)), (p_r - discSize.x)/(discSize.y-discSize.x))).rrr;
    //return vec4(timeDelta, top, 0, alpha);
    */
    //vec3 color = vec3(max(0.0,density)) * discParams.x;
    return vec4(color*discParams.x, alpha*discParams.y);
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
    float alpha = acos(clamp(dot(e_x_prime, t), -1.0, 1.0));
    float delta = acos(clamp(dot(e_x_prime, normalize(dir)), -1.0, 1.0));
    float u = 1.0 / length(pos);
    float u_dot = -u / tan(delta);
    float e_square = u_dot * u_dot + u * u * (1.0 - u);
    float e = -sqrt(e_square);


    float u0, phi0, t0, u1, phi1, t1;
    float deflection = RayTrace(u, u_dot, e_square, delta, alpha,
        u0, phi0, t0, u1, phi1, t1);

    vec4 l = vec4(e / (1.0 - u), -u_dot, 0.0, u * u);
    float g_k_l_receiver = ks.x * l.x * (1.0 - u) - ks.y * l.y / (1.0 - u) -
                         u * dot(etau, e_y_prime) * l.w / (u * u);
    vec3 color = vec3(0,0,0);

    if (deflection >= 0.0) {
        float delta_prime = delta + max(deflection, 0.0);
        vec3 d_prime = cos(delta_prime) * e_x_prime + sin(delta_prime) * e_y_prime;
        d_prime = vec3(d_prime.x, -d_prime.z, d_prime.y);
        color = texture(cubeMap, d_prime).rgb;
    }

#ifdef DISC
    if (u1 >= 0.0 && u1 < discSize.z && u1 > discSize.w) {
        float g_k_l_source = e * sqrt(2.0 / (2.0 - 3.0 * u1)) -
                         u1 * sqrt(u1 / (2.0 - 3.0 * u1)) * dot(e_z, e_z_prime);
        #ifdef DOPPLER
        float z_grav = sqrt((1-u1)/(1-u));
        float doppler_factor = z_grav * g_k_l_receiver/g_k_l_source;
        #else
        float doppler_factor = 1.0;
        #endif
        bool top_side =
            (mod(abs(phi1 - alpha), 2.0 * pi) < 1e-3) == (e_x_prime.z > 0.0);
        vec3 intersect = (e_x_prime * cos(phi1) + e_y_prime * sin(phi1))/u1;
        vec4 disc_color = DiscColor(intersect.xy, dt-t1, top_side, doppler_factor);
        color = color * (1.0-disc_color.a) + disc_color.a * disc_color.rgb;
    }
    if (u0 >= 0.0 && u0 < discSize.z && u0 > discSize.w) {
        float g_k_l_source = e * sqrt(2.0 / (2.0 - 3.0 * u0)) -
                         u0 * sqrt(u0 / (2.0 - 3.0 * u0)) * dot(e_z, e_z_prime);
        #ifdef DOPPLER
        float z_grav = sqrt((1-u0)/(1-u));
        float doppler_factor = z_grav * g_k_l_receiver/g_k_l_source;
        #else
        float doppler_factor = 1.0;
        #endif
        bool top_side =
            (mod(abs(phi0 - alpha), 2.0 * pi) < 1e-3) == (e_x_prime.z > 0.0);
        vec3 intersect = (e_x_prime * cos(phi0) + e_y_prime * sin(phi0))/u0;
        vec4 disc_color = DiscColor(intersect.xy, dt-t0, top_side, doppler_factor);
        color = color * (1.0-disc_color.a) + disc_color.a * disc_color.rgb;    
    }
#endif
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
    dir = vec3(dir.x, -dir.z, dir.y);
    vec4 k = vec4(ks.x, -ks.w, ks.y, ks.z);
    vec3 pos = vec3(cameraPos.x, -cameraPos.z, cameraPos.y);
    vec3 tau = vec3(cam_tau.x, -cam_tau.z, cam_tau.y);

    vec3 color = pixelColor(dir, pos, tau, k, dt*10);

    FragColor = vec4(color, 1.0); 
    //FragColor = vec4((texture(noise_texture, TexCoords).rrr),1);
}