#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushConsts {
    mat4 model;
    uint idOffset;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in uint inSelected;

layout(location = 0) out flat uint outId;

void main() {
    gl_Position = ubo.proj * ubo.view * pc.model * vec4(inPosition, 1.0);
    gl_PointSize = 3.0;              
    outId = pc.idOffset + gl_VertexIndex + 1u; // +1 so 0 stays reserved for "nothing"
}