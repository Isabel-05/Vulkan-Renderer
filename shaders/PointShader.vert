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

layout(location = 0) out flat uint outSelected;

void main() {
    gl_Position = ubo.proj * ubo.view * pc.model * vec4(inPosition, 1.0);
    gl_PointSize = 8.0;
    outSelected = inSelected;
}