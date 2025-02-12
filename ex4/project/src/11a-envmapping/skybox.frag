#version 330 core
out vec4 FragColor;
in vec3 WorldPos;

uniform samplerCube skybox;

void main()
{    

    // Todo: cube texture lookup
    FragColor = vec4(WorldPos,1.0);
}