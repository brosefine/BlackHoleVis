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


out vec4 FragColor;

in vec3 cameraPos;
in vec3 viewDir;
in vec2 TexCoords;

layout(binding = 0) uniform sampler2D deflection_texture;
layout(binding = 1) uniform sampler2D inv_radius_texture;

uniform vec3 cam_tau;
uniform vec3 cam_up;
uniform vec3 cam_front;
uniform vec3 cam_right;
uniform vec4 ks;
uniform vec2 discSize;

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

void RayTrace(float u, float u_dot, float e_square, float delta, float alpha,
    out float u0, out float phi0, out float t0, 
    out float u1, out float phi1, out float t1){
    
    u0 = -1.0;
    u1 = -1.0;
    if (e_square < kMu && u > 2.0 / 3.0) {
        return;
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
}

vec3 pixelColor(vec3 dir, vec3 pos, vec3 etau, vec4 ks) {
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
    RayTrace(u, u_dot, e_square, delta, alpha,
        u0, phi0, t0, u1, phi1, t1);

/*
    vec4 l = vec4(e / (1.0 - u), -u_dot, 0.0, u * u);
    float g_k_l_receiver = ks.x * l.x * (1.0 - u) - ks.y * l.y / (1.0 - u) -
                         u * dot(etau, e_y_prime) * l.w / (u * u);
*/
    vec3 color = vec3(0,0,0);

    if (u1 >= 0.0 && u1 < discSize.x && u1 > discSize.y) {
        color = vec3(0.5, 0, 0);
    }
    if (u0 >= 0.0 && u0 < 1.0/3.0 && u0 > 1.0/12.0) {
        color = 0.5 * color + vec3(0, 0.5, 0); 
    }
    return color;

}

void main()
{    
    vec3 dir = normalize(viewDir);
    /*
    float phi = (1.0 - TexCoords.x) * 2.0 * pi;
    float theta = (1.0 - TexCoords.y) * pi;
    dir.x = sin(phi) * sin(theta);
    dir.y = cos(theta);
    dir.z = cos(phi) * sin(theta);
    */
    //FragColor = vec4(abs((dir)),1);
    //return;
    dir = normalize(-cam_tau + dir.x * cam_right + dir.y * cam_up - dir.z * cam_front);

    dir = vec3(dir.x, -dir.z, dir.y);
    vec4 k = vec4(ks.x, -ks.w, ks.y, ks.z);
    vec3 pos = vec3(cameraPos.x, -cameraPos.z, cameraPos.y);
    vec3 tau = vec3(cam_tau.x, -cam_tau.z, cam_tau.y);

    vec3 color = pixelColor(dir, pos, tau, k);

    FragColor = vec4(color, 1.0); 
    //FragColor = vec4(2*texture(doppler_texture, vec3(TexCoords, 0.5)).rgb,1);
    //FragColor = vec4(100*texture(doppler_texture, TexCoords).rgb,1);
}