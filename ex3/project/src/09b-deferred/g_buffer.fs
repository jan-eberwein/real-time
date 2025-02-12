#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;


in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;

void main()
{    
    // store the fragment position vector in the first gbuffer texture
    gPosition = vec3(0.0,1.0,0.0);
    // also store the per-fragment normals into the gbuffer
    gNormal = vec3(0.0,0.0,1.0);
    // and the diffuse per-fragment color
    gAlbedoSpec.rgb = vec3(1.0,0.0,0.0);
    // store specular intensity in gAlbedoSpec's alpha component
    gAlbedoSpec.a = 0.5;
}