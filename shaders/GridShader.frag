#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushConsts {
    mat4 invViewProj;
    vec3  cameraPos;
    float cellSize;
    float fadeNear;
    float fadeFar;
} pc;

layout(location = 0) in vec2 inNDC;
layout(location = 0) out vec4 outColor;

vec3 unproject(vec2 ndcXY, float z)
{
    vec4 p = pc.invViewProj * vec4(ndcXY, z, 1.0);
    return p.xyz / p.w;
}

// 0 = not on a line, 1 = dead center, antialiased to ~1px regardless of distance
float gridLine(vec2 coord, float lineWidthPixels)
{
    vec2 deriv = fwidth(coord);
    vec2 dist  = abs(fract(coord - 0.5) - 0.5) / max(deriv, vec2(1e-7));
    float line = min(dist.x, dist.y);
    return 1.0 - clamp(line / lineWidthPixels, 0.0, 1.0);
}

void main()
{
    vec3 nearP = unproject(inNDC, 0.0);
    vec3 farP  = unproject(inNDC, 1.0);

    float denom = farP.y - nearP.y;
    if (abs(denom) < 1e-6)
        discard;

    float t = -nearP.y / denom;
    if (t < 0.0 || t > 1.0)
        discard; // plane crossing is behind the camera, or past the far clip

    vec3 hit = nearP + t * (farP - nearP);

    float dist = length(hit - pc.cameraPos);
    float fade = 1.0 - smoothstep(pc.fadeNear, pc.fadeFar, dist);
    if (fade <= 0.001)
        discard;

    vec2 minorCoord = hit.xz / pc.cellSize;
    vec2 majorCoord = hit.xz / (pc.cellSize * 10.0);

    float minor = gridLine(minorCoord, 1.0);
    float major = gridLine(majorCoord, 1.5);

    vec3 color  = vec3(0.35);
    float alpha = minor;

    if (major > alpha)
    {
        color = vec3(0.55);
        alpha = major;
    }

    // axis highlights: world x==0 is the Z axis (blue), world z==0 is the X axis (red)
    float zAxisWidth = max(fwidth(hit.x) * 2.0, 1e-6);
    float xAxisWidth = max(fwidth(hit.z) * 2.0, 1e-6);
    float onZAxis = 1.0 - clamp(abs(hit.x) / zAxisWidth, 0.0, 1.0);
    float onXAxis = 1.0 - clamp(abs(hit.z) / xAxisWidth, 0.0, 1.0);

    if (onZAxis > alpha) { color = vec3(0.2, 0.3, 0.65); alpha = onZAxis; }
    if (onXAxis > alpha) { color = vec3(0.65, 0.2, 0.2); alpha = onXAxis; }

    outColor = vec4(color, alpha * fade);

    vec4 clip = ubo.proj * ubo.view * vec4(hit, 1.0);
    gl_FragDepth = clip.z / clip.w;
}