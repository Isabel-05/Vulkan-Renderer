#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 greycolor = vec4(0.3, 0.3, 0.3, 1.0);
    vec3 lightdir = vec3(-1.0, -0.5, -1.0);
    outColor = (dot(lightdir, fragNormal)) * greycolor;
}