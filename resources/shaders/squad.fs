out vec4 FragColor;

in vec2 TexCoords;

layout(binding = 0) uniform sampler2D fboTexture;
layout(binding = 1) uniform sampler2D bloomTexture;

uniform bool bloom = false;
uniform bool tonemap = false;

uniform float exposure = 1;
uniform float gamma = 2.2;

// ACES tone map, see
  // https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
  vec3 toneMapACES(vec3 color) {
    const float A = 2.51;
    const float B = 0.03;
    const float C = 2.43;
    const float D = 0.59;
    const float E = 0.14;
    color = (color * (A * color + B)) / (color * (C * color + D) + E);
    return pow(color, vec3(1.0 / gamma));
}

void main()
{    
    
    vec3 color = texture(fboTexture, TexCoords).rgb;
    if(bloom)
        color += texture(bloomTexture, TexCoords).rgb;
    
    color *= exposure;
    if(tonemap)
      color = toneMapACES(color);
    FragColor = vec4(color, 1.0);
    //FragColor = vec4(TexCoords, 0.0, 1.0);
    
}