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
layout(binding = 2) uniform sampler2D mw_panorama;
layout(binding = 3) uniform samplerCube star_texture;
layout(binding = 4) uniform samplerCube star_texture2;

uniform vec3 cam_tau;
uniform vec3 cam_up;
uniform vec3 cam_front;
uniform vec3 cam_right;

uniform vec3 boosted_tau = vec3(0);
uniform vec3 boosted_up = vec3(0, 0, 1);
uniform vec3 boosted_front = vec3(0, 1, 0);
uniform vec3 boosted_right = vec3(1, 0, 0);

uniform float star_exposure = 1.0;

uniform bool gaiaMap = false;

const float MAX_FOOTPRINT_LOD = 6.0;
const int STARS_CUBE_MAP_SIZE = 2048;
const float MAX_FOOTPRINT_SIZE = 4.0;

const float pi = 3.14159265359;
const float pi2 = 2.0 * pi;
const float pi_1_2 = 0.5 * pi;
const float i_pi = 1.0 / pi;
const float i_pi2 = 1.0 / pi2;

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

// factor = 0 -> a, factor = 1 -> b
vec2 interpolateThPhi(vec2 a, vec2 b, float factor) {
  if (all(lessThan(a, vec2(0))))
    return b;
  if (all(lessThan(b, vec2(0))))
    return a;

  return mix(a, b, factor);
}

vec3 pixelColor(vec3 dir) {
    
    // get phi, theta from dir
    float phi = 1.0 - mod(atan(dir.y,dir.x) + 1.5*pi, pi2)/ (pi2);
    float theta = 1.0 - mod(atan(sqrt(dir.x*dir.x+dir.y*dir.y)/dir.z), pi) * i_pi;

    vec2 new_thphi = texture(deflection_texture, vec2(phi, theta)).xy;

    // custom linear sampling which ignores black hole pixels
    // p10 --- p11
    //  | p     |
    // p00 --- p01
    #ifdef LINEARSAMPLE
    if (all(greaterThanEqual(new_thphi, vec2(0)))) {

      // texel size
      vec2 txlSize = 1.0 / vec2(textureSize(deflection_texture, 0));
      vec2 p = vec2(phi, theta) * textureSize(deflection_texture, 0);
      vec2 p_lowerleft = floor(p - vec2(0.5)) + 0.5;
      vec2 coords_00 = p_lowerleft * txlSize;
      vec2 p_00 = texture(deflection_texture, coords_00).xy;
      vec2 p_01 = texture(deflection_texture, coords_00 + vec2(txlSize.x, 0)).xy;
      vec2 p_10 = texture(deflection_texture, coords_00 + vec2(0, txlSize.y)).xy;
      vec2 p_11 = texture(deflection_texture, coords_00 + txlSize).xy;

      vec2 p_dist = p - p_lowerleft;
      vec2 p_00_01 = interpolateThPhi(p_00, p_01, p_dist.x);
      vec2 p_10_11 = interpolateThPhi(p_10, p_11, p_dist.x);
      new_thphi = interpolateThPhi(p_00_01, p_10_11, p_dist.y);

    }
    #endif // LINEARSAMPLE


    vec3 color = vec3(0);
    if (all(greaterThanEqual(new_thphi, vec2(0.0)))) {
  #ifdef DEFLECTIONMAP
    color = vec3(new_thphi.x * i_pi, new_thphi.y * i_pi2, 0);
  #else // DEFLECTIONMAP

    #ifndef MWPANORAMA
        vec3 d_prime;
        d_prime.x = sin(new_thphi.y) * sin(new_thphi.x);
        d_prime.y = cos(new_thphi.y) * sin(new_thphi.x);
        d_prime.z = cos(new_thphi.x);
        d_prime = normalize(-cam_tau + d_prime.x * cam_right + d_prime.z * cam_up - d_prime.y * cam_front);

        if(gaiaMap) d_prime = vec3(d_prime.x, -d_prime.z, d_prime.y);
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
        color += StarColor(d_prime, lensing_amplification_factor/pixel_area, 0.0) * star_exposure;
        #endif // STARS
    #else // MWPANORAMA
        vec2 mw_coords = new_thphi / vec2(pi, pi2);
        color = texture(mw_panorama, mw_coords.yx).rgb;

    #endif // MWPANORAMA

    #endif // DEFLECTIONMAP
    }

    return color;

}

// functions for rotation using quaternions
// from https://gist.github.com/nkint/7449c893fb7d6b5fa83118b8474d7dcb
vec4 setAxisAngle (vec3 axis, float rad) {
  rad = rad * 0.5;
  float s = sin(rad);
  return vec4(s * axis.x, s * axis.y, s * axis.z, cos(rad));
}

vec4 multQuat(vec4 q1, vec4 q2) {
  return vec4(
    q1.w * q2.x + q1.x * q2.w + q1.z * q2.y - q1.y * q2.z,
    q1.w * q2.y + q1.y * q2.w + q1.x * q2.z - q1.z * q2.x,
    q1.w * q2.z + q1.z * q2.w + q1.y * q2.x - q1.x * q2.y,
    q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z
  );
}

vec3 rotateVector( vec4 quat, vec3 vec ) {
  // https://twistedpairdevelopment.wordpress.com/2013/02/11/rotating-a-vector-by-a-quaternion-in-glsl/
  vec4 qv = multQuat( quat, vec4(vec, 0.0) );
  return multQuat( qv, vec4(-quat.x, -quat.y, -quat.z, quat.w) ).xyz;
}

void main()
{    
    FragColor = vec4(texture(deflection_texture, TexCoords).xy, 0, 1);
    // return;

    #if defined(PINHOLE)
    vec3 dir = normalize(viewDir);
    dir = vec3(-dir.x, dir.z, dir.y);
    #elif defined(DOME)
    
    vec2 fragCoord = 2.0*(TexCoords - vec2(0.5));

    if(length(fragCoord) > 1) {
        FragColor = vec4(0,0,0,1);
        return;
    }

    float theta = (length(fragCoord)) * pi_1_2;
    float phi = atan(fragCoord.y, fragCoord.x) - pi_1_2;
    vec3 dir;
    dir.x = sin(phi) * sin(theta);
    dir.z = cos(theta);
    dir.y = cos(phi) * sin(theta);

    // rotate view direction downwards to move black hole into
    // dome focus area
    vec4 rotQuat = setAxisAngle(vec3(1, 0, 0), -pi/4.0);
    dir = normalize(rotateVector(rotQuat, dir));

    #else
    float phi = (TexCoords.x) * pi2;
    float theta = (1.0 - TexCoords.y) * pi;
    vec3 dir;
    dir.x = sin(phi) * sin(theta);
    dir.y = cos(phi) * sin(theta);
    dir.z = cos(theta);
    #endif    

    dir = normalize(-boosted_tau + dir.x * boosted_right + dir.z * boosted_up + dir.y * boosted_front);

    vec3 color;
    // color = texture(deflection_texture, TexCoords).rgb;
    // color.r /= pi;
    // color.g /= (2*pi);
    // color = texture(mw_panorama, color.gr).rgb;
    color = pixelColor(normalize(dir));
    FragColor = vec4(color, 1.0); 

}