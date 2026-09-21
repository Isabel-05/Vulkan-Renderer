#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main() {

    vec3 normal = normalize(fragNormal);
    vec3 lightdir = normalize(vec3(0.5, -0.3, -1.0));

    float dotp = max(dot(lightdir, normal), 0.0);
    vec3 color = vec3(0.45) * (0.1 + 0.75 * dotp);

    outColor = vec4(color, 1.0);
}