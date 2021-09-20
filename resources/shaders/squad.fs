out vec4 FragColor;

in vec2 TexCoords;

layout(binding = 0) uniform sampler2D fboTexture;
layout(binding = 1) uniform sampler2D bloomTexture;

uniform bool bloom = false;


void main()
{    
    float gamma = 2.2;
    float exposure = 1;
    vec3 color = texture(fboTexture, TexCoords).rgb;
    if(bloom)
        color += texture(bloomTexture, TexCoords).rgb;

    //color = vec3(1.0) - exp(-color * exposure);
    // also gamma correct while we're at it       
    //color = pow(color, vec3(1.0 / gamma));
    FragColor = vec4(color, 1.0);
    
}