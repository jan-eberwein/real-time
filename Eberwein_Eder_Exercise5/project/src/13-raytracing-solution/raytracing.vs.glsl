#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texcoord;

uniform mat4 projection;
uniform mat4 view;
uniform vec3 camPos;
uniform vec2 viewportSize;

out vec3 upOffset;
out vec3 rightOffset;
out vec3 rayOrigin;
out vec3 rayDir;

void main() {
    rayOrigin = camPos;
    mat4 invView = inverse(view);
    vec4 worldPos = inverse(projection * view) * vec4(a_position, 1.0);
    rayDir = (worldPos.xyz / worldPos.w) - rayOrigin;
    upOffset = normalize(invView[1].xyz) * (1.0 / viewportSize.y);
    rightOffset = normalize(invView[0].xyz) * (1.0 / viewportSize.x);
    gl_Position = vec4(a_position, 1.0);
}
