#version 450 core
layout(location = 0) in vec3 aPosition;
layout(location = 2) in vec2 aUv;

uniform mat4 uModel;
uniform mat4 uViewProjection;
uniform float uTime;
uniform float uWaveAmplitude;
uniform float uWaveSpeed;
uniform vec2 uFlowDirection;
uniform int uSurfaceKind;

out VS_OUT {
    vec3 worldPosition;
    vec3 normal;
    vec3 tangent;
    vec2 uv;
    vec2 localPosition;
    float crest;
} vs;

const float PI = 3.14159265359;

float wave(vec2 point, vec2 direction, float wavelength, float speed, float phase) {
    direction = normalize(direction);
    float frequency = 2.0 * PI / max(wavelength, 0.05);
    return sin(frequency * dot(direction, point) - speed * uWaveSpeed * uTime + phase);
}

float surface_height(vec2 point) {
    vec2 flow = length(uFlowDirection) > 0.001 ? normalize(uFlowDirection) : vec2(0.0, 1.0);
    vec2 across = vec2(-flow.y, flow.x);
    float amplitude = max(uWaveAmplitude, 0.0);

    if (uSurfaceKind == 1) {
        return amplitude * (
            wave(point, flow, 1.55, 1.50, 0.0) * 0.48 +
            wave(point, normalize(flow + across * 0.48), 0.72, 2.05, 1.7) * 0.25 +
            wave(point, across, 0.36, 2.60, 3.8) * 0.11);
    }

    if (uSurfaceKind == 2) {
        float radius = length(point);
        float radial = sin(radius * 17.0 - uTime * uWaveSpeed * 2.4);
        float secondary = sin(radius * 8.0 + uTime * uWaveSpeed * 1.15 + 1.6);
        float crossing = wave(point, vec2(0.74, 0.67), 0.78, 1.65, 2.4);
        return amplitude * (radial * 0.52 + secondary * 0.19 + crossing * 0.16);
    }

    if (uSurfaceKind == 3) {
        return amplitude * (
            wave(point, vec2(1.0, 0.18), 3.8, 0.62, 0.0) * 0.46 +
            wave(point, vec2(-0.26, 1.0), 2.15, 0.82, 2.1) * 0.24 +
            wave(point, vec2(0.65, -0.72), 1.15, 1.05, 4.0) * 0.08);
    }

    return amplitude * (
        wave(point, vec2(1.0, 0.26), 3.0, 0.78, 0.0) * 0.52 +
        wave(point, vec2(-0.34, 1.0), 1.55, 1.02, 1.6) * 0.29 +
        wave(point, vec2(0.76, -0.58), 0.78, 1.30, 3.9) * 0.12);
}

void main() {
    vec2 point = aPosition.xz;
    float height = surface_height(point);
    const float epsilon = 0.012;
    float dhdx = (surface_height(point + vec2(epsilon, 0.0)) -
                  surface_height(point - vec2(epsilon, 0.0))) / (2.0 * epsilon);
    float dhdz = (surface_height(point + vec2(0.0, epsilon)) -
                  surface_height(point - vec2(0.0, epsilon))) / (2.0 * epsilon);

    vec3 position = aPosition + vec3(0.0, height, 0.0);
    vec3 localNormal = normalize(vec3(-dhdx, 1.0, -dhdz));
    vec3 localTangent = normalize(vec3(1.0, dhdx, 0.0));

    vec4 world = uModel * vec4(position, 1.0);
    mat3 normalMatrix = transpose(inverse(mat3(uModel)));
    vs.worldPosition = world.xyz;
    vs.normal = normalize(normalMatrix * localNormal);
    vs.tangent = normalize(mat3(uModel) * localTangent);
    vs.uv = aUv;
    vs.localPosition = point;
    vs.crest = clamp(0.5 + height / max(uWaveAmplitude * 1.4, 0.008), 0.0, 1.0);
    gl_Position = uViewProjection * world;
}
