in vec2 TexCoords;

layout(binding=0) uniform sampler2D source;
uniform vec3 texDimLevel;

out vec4 FragColor;

void main() {
    vec3 color = textureLod(source, TexCoords, texDimLevel.z).rgb;
    FragColor = vec4(min(color, 6.55e4), 1.0);

}