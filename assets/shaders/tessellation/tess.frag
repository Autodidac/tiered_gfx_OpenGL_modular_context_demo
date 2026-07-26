#version 450 core
layout(location=0) out vec4 outScene;
layout(location=1) out vec4 outBright;

in TE_OUT { vec3 world; vec3 normal; vec2 uv; vec4 lightPosition; } v;

uniform sampler2D uSplatMap;
uniform sampler2D uAlbedoMaps[4];
uniform sampler2D uNormalMaps[4];
uniform sampler2D uOrmMaps[4];
uniform sampler2D uShadowMap;
uniform vec3 uCameraPosition;
uniform vec3 uSunDirection;
uniform vec3 uSunColor;
uniform float uEnvironmentStrength;
uniform int uShadowsEnabled;
uniform int uFogEnabled;
uniform float uFogDensity;
uniform float uFogHeightFalloff;

float hash21(vec2 value) {
    value = fract(value * vec2(123.34, 456.21));
    value += dot(value, value + 45.32);
    return fract(value.x * value.y);
}

mat2 rotation(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat2(c, -s, s, c);
}

vec4 smooth_splat(vec2 uv) {
    vec2 texel = 1.0 / vec2(textureSize(uSplatMap, 0));
    vec4 result = texture(uSplatMap, uv) * 0.40;
    result += texture(uSplatMap, uv + vec2(texel.x, 0.0)) * 0.15;
    result += texture(uSplatMap, uv - vec2(texel.x, 0.0)) * 0.15;
    result += texture(uSplatMap, uv + vec2(0.0, texel.y)) * 0.15;
    result += texture(uSplatMap, uv - vec2(0.0, texel.y)) * 0.15;
    return max(result, vec4(0.0));
}

vec4 anti_tile(sampler2D source, vec2 uv, float seed) {
    vec2 cell = floor(v.world.xz * 0.085 + seed);
    float selector = hash21(cell + seed);
    vec2 uvA = rotation(selector * 1.7 - 0.85) * uv + vec2(selector, selector * 0.37);
    vec2 uvB = rotation(selector * -1.2 + 0.55) * (uv * 0.73) + vec2(0.31, 0.67) + seed;
    float blend = smoothstep(0.25, 0.75, hash21(cell * 0.73 + vec2(3.1, 7.9)));
    return mix(texture(source, uvA), texture(source, uvB), blend * 0.38);
}

float shadow_visibility(vec3 n, vec3 l) {
    if (uShadowsEnabled == 0) return 1.0;
    vec3 p = v.lightPosition.xyz / max(v.lightPosition.w, 0.00001);
    p = p * 0.5 + 0.5;
    if (p.z <= 0.0 || p.z >= 1.0 || any(lessThan(p.xy, vec2(0.0))) || any(greaterThan(p.xy, vec2(1.0)))) return 1.0;
    float bias = max(0.0013 * (1.0 - max(dot(n, l), 0.0)), 0.00018);
    vec2 texel = 1.0 / vec2(textureSize(uShadowMap, 0));
    float visible = 0.0;
    for (int y = -1; y <= 1; ++y)
        for (int x = -1; x <= 1; ++x)
            visible += p.z - bias <= texture(uShadowMap, p.xy + vec2(x, y) * texel).r ? 1.0 : 0.0;
    return visible / 9.0;
}

void main() {
    vec4 weights = smooth_splat(v.uv);
    weights /= max(dot(weights, vec4(1.0)), 0.0001);

    // Meter-scale detail plus anti-tiling keeps the 64 m terrain free of checkerboard repetition.
    vec2 tiled = v.world.xz * 0.28;
    vec3 albedo = vec3(0.0);
    vec3 tangentNormal = vec3(0.0);
    vec3 orm = vec3(0.0);
    for (int i = 0; i < 4; ++i) {
        float seed = float(i) * 1.83;
        albedo += anti_tile(uAlbedoMaps[i], tiled, seed).rgb * weights[i];
        tangentNormal += (anti_tile(uNormalMaps[i], tiled, seed).xyz * 2.0 - 1.0) * weights[i];
        orm += anti_tile(uOrmMaps[i], tiled, seed).rgb * weights[i];
    }

    float macro = 0.92 + 0.08 * sin(v.world.x * 0.19 + sin(v.world.z * 0.11))
                       * sin(v.world.z * 0.17 - sin(v.world.x * 0.09));
    albedo *= macro;

    vec3 baseNormal = normalize(v.normal);
    vec3 tangent = normalize(vec3(1.0, 0.0, 0.0) - baseNormal * baseNormal.x);
    if (length(tangent) < 0.1) tangent = normalize(cross(vec3(0,0,1), baseNormal));
    vec3 bitangent = normalize(cross(baseNormal, tangent));
    tangentNormal.xy *= 0.46;
    vec3 n = normalize(mat3(tangent, bitangent, baseNormal) * normalize(tangentNormal));

    vec3 l = normalize(-uSunDirection);
    vec3 viewDir = normalize(uCameraPosition - v.world);
    vec3 h = normalize(l + viewDir);
    float roughness = clamp(orm.g, 0.36, 1.0);
    float diffuse = max(dot(n, l), 0.0);
    float spec = pow(max(dot(n, h), 0.0), mix(72.0, 8.0, roughness)) * 0.035;
    float shadow = shadow_visibility(n, l);
    vec3 color = albedo * (0.085 + uEnvironmentStrength * 0.14)
               + albedo * diffuse * uSunColor * 0.42 * shadow
               + spec * uSunColor * shadow;

    if (uFogEnabled != 0) {
        float distanceToCamera = length(uCameraPosition - v.world);
        float heightDensity = exp(-max(v.world.y, 0.0) * uFogHeightFalloff);
        float fog = clamp(1.0 - exp(-distanceToCamera * uFogDensity * mix(0.55, 1.0, heightDensity)), 0.0, 0.68);
        float horizon = pow(1.0 - abs(clamp(viewDir.y, -1.0, 1.0)), 2.0);
        vec3 fogColor = mix(vec3(0.34, 0.40, 0.47), vec3(0.20, 0.24, 0.29), horizon * 0.62);
        color = mix(color, fogColor, fog);
    }

    outScene = vec4(color, 1.0);
    outBright = vec4(0.0, 0.0, 0.0, 1.0);
}
