#version 450

layout(set = 0, binding = 0) uniform usampler2D idTex;

layout(push_constant) uniform PushConsts {
    uint  selectedId;
    int   thickness;
    ivec2 texSize;
} pc;

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

void main() {
    ivec2 coord = ivec2(fragUV * vec2(pc.texSize));

    uint centerId = texelFetch(idTex, coord, 0).r;

    // never draw over the silhouette itself, only the ring around it
    if (centerId == pc.selectedId) {
        discard;
    }

    int t = max(pc.thickness, 1);
    bool nearSelected = false;

    for (int dy = -t; dy <= t && !nearSelected; dy++) {
        for (int dx = -t; dx <= t && !nearSelected; dx++) {
            if (dx == 0 && dy == 0) continue;

            ivec2 sampleCoord = clamp(coord + ivec2(dx, dy), ivec2(0), pc.texSize - ivec2(1));
            if (texelFetch(idTex, sampleCoord, 0).r == pc.selectedId) {
                nearSelected = true;
            }
        }
    }

    if (!nearSelected) {
        discard;
    }

    outColor = vec4(1.0, 1.0, 1.0, 0.0);
}