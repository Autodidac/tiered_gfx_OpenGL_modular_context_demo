#version 450 core
layout(location=0) out vec4 outScene;
layout(location=1) out vec4 outBright;

in VS_OUT {
    vec3 worldPosition;
    vec2 uv;
    mat3 tbn;
    vec4 lightPosition;
} fs;

uniform sampler2D uAlbedo;
uniform sampler2D uNormal;
uniform sampler2D uOrm;
uniform sampler2D uShadowMap;
uniform samplerCube uEnvironment;
uniform vec3 uCameraPosition;
uniform vec3 uSunDirection;
uniform vec3 uSunColor;
uniform vec3 uTint;
uniform float uEnvironmentStrength;
uniform int uShadowsEnabled;
uniform int uFogEnabled;
uniform float uFogDensity;
uniform float uFogHeightFalloff;

float shadow_visibility(vec3 normal, vec3 lightDirection) {
    if (uShadowsEnabled == 0) return 1.0;
    vec3 projected = fs.lightPosition.xyz / max(fs.lightPosition.w, 0.00001);
    projected = projected * 0.5 + 0.5;
    if (projected.z <= 0.0 || projected.z >= 1.0 ||
        any(lessThan(projected.xy, vec2(0.0))) || any(greaterThan(projected.xy, vec2(1.0)))) return 1.0;
    float bias = max(0.0012 * (1.0 - max(dot(normal, lightDirection), 0.0)), 0.0002);
    vec2 texel = 1.0 / vec2(textureSize(uShadowMap, 0));
    float visible = 0.0;
    for (int y = -1; y <= 1; ++y)
        for (int x = -1; x <= 1; ++x)
            visible += projected.z - bias <= texture(uShadowMap, projected.xy + vec2(x, y) * texel).r ? 1.0 : 0.0;
    return visible / 9.0;
}

void main() {
    vec3 albedo = texture(uAlbedo, fs.uv).rgb * uTint;
    vec3 orm = texture(uOrm, fs.uv).rgb;
    float roughness = clamp(orm.g * 1.18, 0.35, 1.0);
    vec3 tangentNormal = texture(uNormal, fs.uv).xyz * 2.0 - 1.0;
    tangentNormal.xy *= 0.45;
    vec3 normal = normalize(fs.tbn * tangentNormal);
    if (!gl_FrontFacing) normal = -normal;

    vec3 viewDirection = normalize(uCameraPosition - fs.worldPosition);
    vec3 lightDirection = normalize(-uSunDirection);
    vec3 halfVector = normalize(viewDirection + lightDirection);
    float diffuse = max(dot(normal, lightDirection), 0.0);
    float specular = pow(max(dot(normal, halfVector), 0.0), mix(42.0, 8.0, roughness)) * 0.12;
    float shadow = shadow_visibility(normal, lightDirection);
    vec3 environment = textureLod(uEnvironment, reflect(-viewDirection, normal), roughness * 5.0).rgb;
    vec3 color = albedo * (0.09 + diffuse * shadow * uSunColor)
        + environment * uEnvironmentStrength * 0.08
        + uSunColor * specular * shadow;
    if (uFogEnabled != 0) {
        float distanceToCamera = length(uCameraPosition - fs.worldPosition);
        float heightDensity = exp(-max(fs.worldPosition.y, 0.0) * uFogHeightFalloff);
        float fog = clamp(1.0 - exp(-distanceToCamera * uFogDensity * mix(0.55, 1.0, heightDensity)), 0.0, 0.80);
        float horizon = pow(1.0 - abs(clamp(viewDirection.y, -1.0, 1.0)), 2.0);
        vec3 fogColor = mix(vec3(0.58, 0.66, 0.74), vec3(0.34, 0.40, 0.47), horizon * 0.62);
        color = mix(color, fogColor, fog);
    }

    outScene = vec4(color, 1.0);
    outBright = vec4(0.0, 0.0, 0.0, 1.0);
}
