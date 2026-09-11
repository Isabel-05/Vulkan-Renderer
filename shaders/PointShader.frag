#version 450

layout(location = 0) out vec4 outColor;

void main() {
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord);

    // discard fragments outside the circle (radius 0.5)
    if (dist > 0.5) {
        discard;
    }

    outColor = vec4(0.0, 0.0, 0.0, 1.0);
}