#version 450 core
layout(location = 0) out vec4 outScene;
layout(location = 1) out vec4 outBright;

in VS_OUT {
    vec3 worldPosition;
    vec3 normal;
    vec3 tangent;
    vec2 uv;
    vec2 localPosition;
    float crest;
} fs;

uniform sampler2D uNormalMap;
uniform sampler2D uFoamMap;
uniform samplerCube uEnvironmentMap;
uniform sampler2D uSceneColor;
uniform sampler2D uSceneDepth;
uniform vec2 uViewportSize;
uniform vec3 uCameraPosition;
uniform vec3 uSunDirection;
uniform vec3 uSunColor;
uniform vec3 uShallowColor;
uniform vec3 uDeepColor;
uniform vec2 uFlowDirection;
uniform float uEnvironmentStrength;
uniform float uTime;
uniform float uRoughness;
uniform float uOpacity;
uniform float uFoamStrength;
uniform float uNearPlane;
uniform float uFarPlane;
uniform float uRefractionStrength;
uniform int uSurfaceKind;
uniform int uFogEnabled;
uniform float uFogDensity;
uniform float uFogHeightFalloff;

float linear_depth(float rawDepth) {
    float z = rawDepth * 2.0 - 1.0;
    return (2.0 * uNearPlane * uFarPlane) /
        max(uFarPlane + uNearPlane - z * (uFarPlane - uNearPlane), 0.00001);
}

vec3 fog_color(vec3 viewDirection) {
    float horizon = pow(1.0 - abs(clamp(viewDirection.y, -1.0, 1.0)), 2.0);
    return mix(vec3(0.34, 0.40, 0.47), vec3(0.20, 0.24, 0.29), horizon * 0.68);
}

float fog_amount(float distanceToCamera, float worldHeight) {
    float heightDensity = exp(-max(worldHeight, 0.0) * uFogHeightFalloff);
    float density = uFogDensity * mix(0.50, 1.0, heightDensity);
    return clamp(1.0 - exp(-distanceToCamera * density), 0.0, 0.68);
}

float distance_to_edge() {
    if (uSurfaceKind == 1 || uSurfaceKind == 3) {
        vec2 q = abs(fs.localPosition);
        return min(1.0 - q.x, 1.0 - q.y);
    }
    return 1.0 - length(fs.localPosition);
}

void main() {
    float edgeDistance = distance_to_edge();
    if (edgeDistance < 0.0) discard;

    vec3 viewDirection = normalize(uCameraPosition - fs.worldPosition);
    vec3 tangent = normalize(fs.tangent - fs.normal * dot(fs.normal, fs.tangent));
    vec3 bitangent = normalize(cross(fs.normal, tangent));
    mat3 tbn = mat3(tangent, bitangent, fs.normal);

    vec2 flow = length(uFlowDirection) > 0.001 ? normalize(uFlowDirection) : vec2(0.0, 1.0);
    vec2 across = vec2(-flow.y, flow.x);
    float flowRate = uSurfaceKind == 1 ? 0.048 : uSurfaceKind == 2 ? 0.014 : uSurfaceKind == 3 ? 0.005 : 0.008;
    float detailScale = uSurfaceKind == 1 ? 2.2 : uSurfaceKind == 2 ? 2.7 : uSurfaceKind == 3 ? 1.25 : 1.55;
    vec2 uvA = fs.uv * detailScale + flow * uTime * flowRate;
    vec2 uvB = fs.uv * detailScale * 1.71 - flow * uTime * flowRate * 0.61 + across * uTime * 0.009;
    vec3 detailA = texture(uNormalMap, uvA).xyz * 2.0 - 1.0;
    vec3 detailB = texture(uNormalMap, uvB).xyz * 2.0 - 1.0;
    float detailStrength = uSurfaceKind == 1 ? 0.23 : uSurfaceKind == 2 ? 0.18 : uSurfaceKind == 3 ? 0.16 : 0.15;
    vec3 detail = normalize(vec3((detailA.xy + detailB.xy * 0.55) * detailStrength, 1.0));
    vec3 normal = normalize(tbn * detail);

    float ndv = clamp(dot(normal, viewDirection), 0.0, 1.0);
    float fresnel = 0.0204 + 0.9796 * pow(1.0 - ndv, 5.0);
    vec3 reflectionDirection = reflect(-viewDirection, normal);
    vec3 reflection = textureLod(uEnvironmentMap, reflectionDirection,
        clamp(uRoughness * 5.0, 0.0, 5.0)).rgb;

    vec2 screenUv = gl_FragCoord.xy / max(uViewportSize, vec2(1.0));
    float sceneDepth = linear_depth(texture(uSceneDepth, screenUv).r);
    float waterDepth = linear_depth(gl_FragCoord.z);
    float thickness = clamp(sceneDepth - waterDepth, 0.0, 18.0);

    float baseDistortion = uSurfaceKind == 1 ? 0.014 : uSurfaceKind == 2 ? 0.011 : uSurfaceKind == 3 ? 0.008 : 0.009;
    float thicknessGate = smoothstep(0.02, 0.65, thickness);
    vec2 distortion = normal.xz * baseDistortion * uRefractionStrength
        * mix(0.25, 1.0, 1.0 - ndv) * thicknessGate;
    vec2 refractedUv = clamp(screenUv + distortion, vec2(0.002), vec2(0.998));
    vec3 refractedScene = texture(uSceneColor, refractedUv).rgb;

    float depthBlend = 1.0 - exp(-thickness * (uSurfaceKind == 3 ? 0.36 : 0.24));
    vec3 waterTint = mix(uShallowColor, uDeepColor, clamp(depthBlend, 0.0, 1.0));
    vec3 absorption = exp(-max(vec3(0.02), vec3(0.23, 0.10, 0.055) * thickness));
    vec3 transmission = refractedScene * absorption + waterTint * (1.0 - absorption) * 0.78;

    vec3 lightDirection = normalize(-uSunDirection);
    float diffuse = max(dot(normal, lightDirection), 0.0);
    vec3 halfVector = normalize(viewDirection + lightDirection);
    float specularPower = mix(220.0, 38.0, clamp(uRoughness, 0.0, 1.0));
    float sunSpecular = pow(max(dot(normal, halfVector), 0.0), specularPower);
    vec3 direct = waterTint * uSunColor * diffuse * 0.035 + uSunColor * sunSpecular * 0.72;

    float foamNoise = texture(uFoamMap, fs.uv * 0.92 + flow * uTime * 0.016).r;
    float geometryFoam = (1.0 - smoothstep(0.02, 0.34, thickness)) * smoothstep(0.38, 0.78, foamNoise);
    float edgeFoam = smoothstep(0.16, 0.018, edgeDistance) * smoothstep(0.44, 0.78, foamNoise);
    float crestFoam = smoothstep(0.82, 1.0, fs.crest + (foamNoise - 0.5) * 0.12);
    float foam = clamp((geometryFoam * 0.72 + edgeFoam * 0.36 + crestFoam * 0.18) * uFoamStrength, 0.0, 0.62);

    vec3 waterColor = transmission + direct;
    waterColor = mix(waterColor, reflection * (0.90 + uEnvironmentStrength * 0.45), clamp(fresnel * 0.94, 0.0, 0.92));
    waterColor = mix(waterColor, vec3(0.74, 0.79, 0.80), foam);

    float waterCoverage = smoothstep(0.0, (uSurfaceKind == 1 || uSurfaceKind == 3) ? 0.035 : 0.08, edgeDistance);
    waterCoverage *= clamp(uOpacity, 0.0, 1.0);
    vec3 color = mix(refractedScene, waterColor, waterCoverage);

    if (uFogEnabled != 0) {
        float distanceToCamera = length(uCameraPosition - fs.worldPosition);
        color = mix(color, fog_color(viewDirection), fog_amount(distanceToCamera, fs.worldPosition.y));
    }

    outScene = vec4(color, 1.0);
    float highlight = max(sunSpecular - 0.18, 0.0) * 0.72 + foam * 0.22;
    outBright = vec4(color * highlight, 1.0);
}
