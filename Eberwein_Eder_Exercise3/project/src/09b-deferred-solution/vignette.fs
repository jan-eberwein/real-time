#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D screenTexture;
uniform bool vignetteOn;
uniform float vignetteStrength; // 0.0 = no effect, 1.0 = full effect

void main()
{
    vec4 color = texture(screenTexture, TexCoords);

    if(vignetteOn)
    {
        // Calculate distance from the center of the screen
        float dist = distance(TexCoords, vec2(0.5, 0.5));
        // Start darkening earlier by using 0.3 as the inner threshold
        float vignette = 1.0 - smoothstep(0.3, 0.8, dist);
        // Optionally, multiply to boost the darkening effect further
        vignette = clamp(vignette * 1.5, 0.0, 1.0);
        // Mix the original color with the darkened version
        color.rgb = mix(color.rgb, color.rgb * vignette, vignetteStrength);
    }

    FragColor = color;
}
