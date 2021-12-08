in vec2 TexCoords;

layout(binding=0) uniform sampler2D source;
layout(binding=1) uniform sampler2D bloom;

uniform vec2 texDim;
uniform float intensity = 0.3;
uniform float exposure = 1.0;

out vec4 FragColor;

// ACES tone map, see
// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 toneMapACES(vec3 color) {
    const float A = 2.51;
    const float B = 0.03;
    const float C = 2.43;
    const float D = 0.59;
    const float E = 0.14;
    color = (color * (A * color + B)) / (color * (C * color + D) + E);
    return pow(color, vec3(1.0 / 2.2));
}

void main() {
    vec3 color = textureLod(bloom, TexCoords, 1).rgb;
    color = mix(textureLod(source, TexCoords, 0).rgb, color, intensity) * exposure;
    color = min(color, 10.0);
    FragColor = vec4(toneMapACES(color), 1.0);
}