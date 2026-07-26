#version 450 core
layout(location = 0) out vec4 outColor;
in vec2 uv;

uniform sampler2D uScene;
uniform sampler2D uBloom;
uniform sampler2D uDepth;
uniform vec2 uInvResolution;
uniform mat4 uProjection;
uniform float uExposure;
uniform float uGamma;
uniform float uBloomStrength;
uniform int uBloomEnabled;
uniform int uFxaaEnabled;
uniform int uSsaoEnabled;
uniform float uSsaoStrength;
uniform float uSsaoRadius;
uniform float uNearPlane;
uniform float uFarPlane;

float luma(vec3 color) {
    return dot(color, vec3(0.299, 0.587, 0.114));
}

vec3 fxaa(vec2 coordinate) {
    vec3 northwest = texture(uScene, coordinate + vec2(-1.0, -1.0) * uInvResolution).rgb;
    vec3 northeast = texture(uScene, coordinate + vec2( 1.0, -1.0) * uInvResolution).rgb;
    vec3 southwest = texture(uScene, coordinate + vec2(-1.0,  1.0) * uInvResolution).rgb;
    vec3 southeast = texture(uScene, coordinate + vec2( 1.0,  1.0) * uInvResolution).rgb;
    vec3 middle = texture(uScene, coordinate).rgb;
    float lnw = luma(northwest), lne = luma(northeast);
    float lsw = luma(southwest), lse = luma(southeast), lm = luma(middle);
    float minimum = min(lm, min(min(lnw, lne), min(lsw, lse)));
    float maximum = max(lm, max(max(lnw, lne), max(lsw, lse)));
    vec2 direction = vec2(-((lnw + lne) - (lsw + lse)),
                           ((lnw + lsw) - (lne + lse)));
    float reduction = max((lnw + lne + lsw + lse) * 0.03125, 0.0078125);
    direction = clamp(direction / (min(abs(direction.x), abs(direction.y)) + reduction),
                      vec2(-8.0), vec2(8.0)) * uInvResolution;
    vec3 a = 0.5 * (texture(uScene, coordinate + direction * (1.0 / 3.0 - 0.5)).rgb
                  + texture(uScene, coordinate + direction * (2.0 / 3.0 - 0.5)).rgb);
    vec3 b = a * 0.5 + 0.25 * (texture(uScene, coordinate + direction * -0.5).rgb
                             + texture(uScene, coordinate + direction *  0.5).rgb);
    float lb = luma(b);
    return lb < minimum || lb > maximum ? a : b;
}

vec3 view_position(vec2 coordinate) {
    float depth = texture(uDepth, coordinate).r;
    vec4 clip = vec4(coordinate * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 view = inverse(uProjection) * clip;
    return view.xyz / max(abs(view.w), 0.00001);
}

vec3 view_normal(vec2 coordinate, vec3 center) {
    vec2 dxUv = vec2(uInvResolution.x, 0.0);
    vec2 dyUv = vec2(0.0, uInvResolution.y);
    vec3 left  = view_position(clamp(coordinate - dxUv, vec2(0.001), vec2(0.999)));
    vec3 right = view_position(clamp(coordinate + dxUv, vec2(0.001), vec2(0.999)));
    vec3 down  = view_position(clamp(coordinate - dyUv, vec2(0.001), vec2(0.999)));
    vec3 up    = view_position(clamp(coordinate + dyUv, vec2(0.001), vec2(0.999)));

    // Choose the derivative that does not cross a depth discontinuity. This avoids
    // the black object halos produced by naive depth-only normal reconstruction.
    vec3 dx = abs(right.z - center.z) < abs(center.z - left.z)
        ? right - center : center - left;
    vec3 dy = abs(up.z - center.z) < abs(center.z - down.z)
        ? up - center : center - down;
    vec3 normal = normalize(cross(dx, dy));
    if (normal.z < 0.0) normal = -normal;
    return normal;
}

float interleaved_gradient_noise(vec2 pixel) {
    return fract(52.9829189 * fract(0.06711056 * pixel.x + 0.00583715 * pixel.y));
}

float screen_space_ao(vec2 coordinate) {
    float raw = texture(uDepth, coordinate).r;
    if (raw >= 0.99999) return 1.0;

    vec3 center = view_position(coordinate);
    vec3 normal = view_normal(coordinate, center);
    float viewDistance = max(-center.z, 0.08);
    float pixelRadius = clamp(
        uSsaoRadius * uProjection[1][1] * 0.5 / (viewDistance * uInvResolution.y),
        3.0, 56.0);
    float rotation = interleaved_gradient_noise(gl_FragCoord.xy) * 6.28318530718;
    const float goldenAngle = 2.39996322973;

    float occlusion = 0.0;
    float totalWeight = 0.0;
    for (int index = 0; index < 12; ++index) {
        float fraction = (float(index) + 0.5) / 12.0;
        float angle = rotation + float(index) * goldenAngle;
        vec2 direction = vec2(cos(angle), sin(angle));
        vec2 offset = direction * (sqrt(fraction) * pixelRadius) * uInvResolution;
        vec2 sampleUv = clamp(coordinate + offset, vec2(0.001), vec2(0.999));
        float sampleRaw = texture(uDepth, sampleUv).r;
        if (sampleRaw >= 0.99999) continue;

        vec3 samplePosition = view_position(sampleUv);
        vec3 delta = samplePosition - center;
        float distanceToSample = length(delta);
        if (distanceToSample <= 0.0001 || distanceToSample > uSsaoRadius * 1.6) continue;

        float hemisphere = max(dot(normal, delta / distanceToSample) - 0.075, 0.0);
        float closer = smoothstep(0.0, 0.035 + viewDistance * 0.0025,
                                  samplePosition.z - center.z);
        float rangeWeight = 1.0 - smoothstep(uSsaoRadius * 0.18,
                                             uSsaoRadius * 1.35,
                                             distanceToSample);
        float weight = mix(1.0, 0.55, fraction);
        occlusion += hemisphere * closer * rangeWeight * weight;
        totalWeight += weight;
    }

    float normalized = totalWeight > 0.0 ? occlusion / totalWeight : 0.0;
    float ao = 1.0 - clamp(normalized * uSsaoStrength * 3.8, 0.0, 0.48);
    return clamp(ao, 0.55, 1.0);
}

vec3 aces(vec3 value) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((value * (a * value + b)) / (value * (c * value + d) + e), 0.0, 1.0);
}

void main() {
    vec3 hdr = uFxaaEnabled != 0 ? fxaa(uv) : texture(uScene, uv).rgb;
    if (uSsaoEnabled != 0) hdr *= screen_space_ao(uv);
    if (uBloomEnabled != 0) hdr += texture(uBloom, uv).rgb * uBloomStrength;
    vec3 mapped = aces(hdr * uExposure);
    mapped = pow(mapped, vec3(1.0 / max(uGamma, 0.01)));
    outColor = vec4(mapped, 1.0);
}
