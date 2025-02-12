#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 view;

out vec3 WorldPos;

void main()
{
    WorldPos = aPos;

	// ToDo: remove translation
	vec4 clipPos = projection * view * vec4(WorldPos, 1.0);

	gl_Position = clipPos;
}