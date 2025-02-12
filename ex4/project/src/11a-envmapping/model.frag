#version 330 core
out vec4 fragColor;

in vec3 normal;
in vec2 texCoord;
in vec3 fragPos;

uniform vec3 cameraPos;
uniform samplerCube cubeTex;
uniform int mode = 0;
void main()
{    	
	// Todo: reflection
	vec4 reflectionColor = vec4(fragPos, 1.0);

	// Todo: refraction 
	vec4 refractionColor = vec4(fragPos, 1.0);


	if (mode == 0) // reflection
		fragColor = reflectionColor;
	else if (mode == 1) // refraction 
		fragColor = refractionColor;
 }