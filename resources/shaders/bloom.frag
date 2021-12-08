in vec2 TexCoords;

layout(binding = 0) uniform sampler2D source;
uniform vec3 texDimLevel;
out vec4 FragColor;

// simple 5x5 gauss kernel
// generated with https://dev.theomader.com/gaussian-kernel-calculator/
float weights[25] = float[] (
    0.003765, 0.015019, 0.023792, 0.015019, 0.003765,
    0.015019, 0.059912, 0.094907, 0.059912, 0.015019,
    0.023792, 0.094907, 0.150342, 0.094907, 0.023792,
    0.015019, 0.059912, 0.094907, 0.059912, 0.015019,
    0.003765, 0.015019, 0.023792, 0.015019, 0.003765
);

void main() { 
    vec2 offset = 1.0 / texDimLevel.xy;
    vec3 color = vec3(0.0);
    for (int i = 0; i < 5; ++i) {
        for(int j = 0; j < 5; ++j) {
            color += 
                weights[i*5+j] * 
                textureLod(source, TexCoords + offset * vec2(i-2,j-2), texDimLevel.z).rgb;
        }
    }
    FragColor = vec4(min(color, 6.55e4), 1.0);
}