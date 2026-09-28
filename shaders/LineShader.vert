#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushConsts {
    mat4 model;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in uint inSelected;

void main() {
    gl_Position = ubo.proj * ubo.view * pc.model * vec4(inPosition, 1.0);
}