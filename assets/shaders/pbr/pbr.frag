#version 450 core
layout(location = 0) out vec4 outScene;
layout(location = 1) out vec4 outBright;

in VS_OUT {
    vec3 worldPosition;
    vec2 uv;
    mat3 tbn;
    vec4 lightPosition;
    vec4 projectorPosition;
    vec4 vertexColor;
} fs;

uniform sampler2D uAlbedoMap;
uniform sampler2D uNormalMap;
uniform sampler2D uOrmMap;
uniform sampler2D uHeightMap;
uniform sampler2D uEmissiveMap;
uniform sampler2D uShadowMap;
uniform samplerCube uEnvironmentMap;
uniform sampler2D uOpacityMap;
uniform sampler2D uProjectedTexture;
uniform samplerCube uPointShadowMap;

uniform vec3 uCameraPosition;
uniform vec3 uSunDirection;
uniform vec3 uSunRadiance;
uniform vec4 uPointPositionRadius[4];
uniform vec4 uPointColorIntensity[4];
uniform int uPointLightCount;
uniform int uEditorDebugOverride;
uniform vec4 uEditorDebugColor;
uniform vec4 uSpotPositionRange;
uniform vec4 uSpotDirectionOuter;
uniform vec4 uSpotColorIntensity;
uniform float uSpotInnerCosine;
uniform int uSpotDualSided;
uniform vec4 uSpot2PositionRange;
uniform vec4 uSpot2DirectionOuter;
uniform vec4 uSpot2ColorIntensity;
uniform float uSpot2InnerCosine;
uniform int uSpot2DualSided;
uniform vec4 uSpot3PositionRange;
uniform vec4 uSpot3DirectionOuter;
uniform vec4 uSpot3ColorIntensity;
uniform float uSpot3InnerCosine;
uniform int uSpot3DualSided;

uniform vec3 uBaseColorFactor;
uniform vec3 uEmissiveFactor;
uniform float uMetallicFactor;
uniform float uRoughnessFactor;
uniform float uNormalScale;
uniform float uHeightScale;
uniform float uAlphaCutoff;
uniform float uClearcoatFactor;
uniform float uClearcoatRoughness;
uniform float uTransmissionFactor;
uniform float uIndexOfRefraction;
uniform int uUnlit;
uniform int uReceivesShadow;
uniform uint uMaterialFeatures;

uniform int uDirectionalEnabled;
uniform int uPointLightsEnabled;
uniform int uSpotEnabled;
uniform int uSpot2Enabled;
uniform int uSpot3Enabled;
uniform int uEnvironmentEnabled;
uniform int uNormalMappingEnabled;
uniform int uParallaxEnabled;
uniform int uShadowsEnabled;
uniform int uFogEnabled;
uniform int uToonEnabled;
uniform int uRimEnabled;
uniform int uProjectedTextureEnabled;
uniform int uClearcoatEnabled;
uniform int uReflectionRefractionEnabled;
uniform int uShadingMode;
uniform int uTier0Profile;
uniform int uPointShadowEnabled;
uniform int uPointShadowLightIndex;
uniform float uPointShadowFarPlane;
uniform float uSunIntensity;
uniform float uEnvironmentStrength;
uniform float uGlobalNormalStrength;
uniform float uGlobalParallaxStrength;
uniform float uGlobalClearcoatStrength;
uniform float uGlobalTransmissionStrength;
uniform float uProjectorStrength;
uniform float uFogDensity;
uniform float uFogHeightFalloff;

const float PI = 3.14159265359;
const uint FEATURE_NORMAL_MAPPING = 1u << 0;
const uint FEATURE_PARALLAX_MAPPING = 1u << 1;
const uint FEATURE_ENVIRONMENT = 1u << 2;
const uint FEATURE_CLEARCOAT = 1u << 3;
const uint FEATURE_TRANSMISSION = 1u << 4;
const uint FEATURE_PROJECTED_TEXTURE = 1u << 5;
const uint FEATURE_TOON = 1u << 6;
const uint FEATURE_RIM = 1u << 7;


vec3 fog_color(vec3 viewDirection) {
    float horizon = pow(1.0 - abs(clamp(viewDirection.y, -1.0, 1.0)), 2.0);
    return mix(vec3(0.34, 0.40, 0.47), vec3(0.20, 0.24, 0.29), horizon * 0.62);
}

float fog_amount(float distanceToCamera, float worldHeight) {
    float heightDensity = exp(-max(worldHeight, 0.0) * uFogHeightFalloff);
    return clamp(1.0 - exp(-distanceToCamera * uFogDensity * mix(0.55, 1.0, heightDensity)), 0.0, 0.68);
}

bool material_has(uint feature) {
    return (uMaterialFeatures & feature) != 0u;
}

vec2 parallax_uv(vec2 uv, vec3 viewDirectionTangent) {
    if (uParallaxEnabled == 0 || !material_has(FEATURE_PARALLAX_MAPPING) || uHeightScale <= 0.0001) return uv;
    float viewZ = max(abs(viewDirectionTangent.z), 0.10);
    float layers = uTier0Profile != 0
        ? mix(12.0, 6.0, clamp(viewZ, 0.0, 1.0))
        : mix(28.0, 10.0, clamp(viewZ, 0.0, 1.0));
    float layerDepth = 1.0 / layers;
    vec2 delta = (viewDirectionTangent.xy / viewZ) * (uHeightScale * uGlobalParallaxStrength) / layers;
    vec2 currentUv = uv;
    float currentDepth = 0.0;
    float sampled = 1.0 - texture(uHeightMap, currentUv).r;
    for (int i = 0; i < 32 && currentDepth < sampled; ++i) {
        currentUv -= delta;
        sampled = 1.0 - texture(uHeightMap, currentUv).r;
        currentDepth += layerDepth;
    }
    vec2 previousUv = currentUv + delta;
    float after = sampled - currentDepth;
    float before = (1.0 - texture(uHeightMap, previousUv).r) - currentDepth + layerDepth;
    float weight = after / max(after - before, 0.00001);
    return mix(currentUv, previousUv, clamp(weight, 0.0, 1.0));
}

float distribution_ggx(vec3 n, vec3 h, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float ndh = max(dot(n, h), 0.0);
    float denominator = ndh * ndh * (a2 - 1.0) + 1.0;
    return a2 / max(PI * denominator * denominator, 0.00001);
}
float geometry_schlick_ggx(float ndv, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return ndv / max(ndv * (1.0 - k) + k, 0.00001);
}
float geometry_smith(vec3 n, vec3 v, vec3 l, float roughness) {
    return geometry_schlick_ggx(max(dot(n, v), 0.0), roughness) *
           geometry_schlick_ggx(max(dot(n, l), 0.0), roughness);
}
vec3 fresnel_schlick(float cosine, vec3 f0) {
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cosine, 0.0, 1.0), 5.0);
}
vec3 fresnel_roughness(float cosine, vec3 f0, float roughness) {
    return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(clamp(1.0 - cosine, 0.0, 1.0), 5.0);
}

float shadow_compare(vec2 uv, float receiverDepth) {
    return receiverDepth <= texture(uShadowMap, uv).r ? 1.0 : 0.0;
}

const vec2 SHADOW_POISSON[24] = vec2[](
    vec2(-0.613392,  0.617481), vec2( 0.170019, -0.040254),
    vec2(-0.299417,  0.791925), vec2( 0.645680,  0.493210),
    vec2(-0.651784,  0.717887), vec2( 0.421003,  0.027070),
    vec2(-0.817194, -0.271096), vec2(-0.705374, -0.668203),
    vec2( 0.977050, -0.108615), vec2( 0.063326,  0.142369),
    vec2( 0.203528,  0.214331), vec2(-0.667531,  0.326090),
    vec2(-0.098422, -0.295755), vec2(-0.885922,  0.215369),
    vec2( 0.566637,  0.605213), vec2( 0.039766, -0.396100),
    vec2( 0.751946,  0.453352), vec2( 0.078707, -0.715323),
    vec2(-0.075838, -0.529344), vec2( 0.724479, -0.580798),
    vec2( 0.222999, -0.215125), vec2(-0.467574, -0.405438),
    vec2(-0.248268, -0.814753), vec2( 0.354411, -0.887570)
);

float shadow_visibility(vec3 normal, vec3 toSun) {
    if (uShadowsEnabled == 0 || uReceivesShadow == 0) return 1.0;
    vec3 projected = fs.lightPosition.xyz / max(fs.lightPosition.w, 0.00001);
    projected = projected * 0.5 + 0.5;
    if (projected.z <= 0.0 || projected.z >= 1.0
        || any(lessThan(projected.xy, vec2(0.0)))
        || any(greaterThan(projected.xy, vec2(1.0)))) return 1.0;

    float slope = 1.0 - max(dot(normal, toSun), 0.0);
    float receiverDepth = projected.z - max(0.0011 * slope, 0.00016);
    vec2 texel = 1.0 / vec2(textureSize(uShadowMap, 0));

    // Tier 0: bounded 3x3 percentage-closer filtering for mobile/Switch budgets.
    if (uTier0Profile != 0) {
        float visible = 0.0;
        for (int y = -1; y <= 1; ++y)
            for (int x = -1; x <= 1; ++x)
                visible += shadow_compare(projected.xy + vec2(x, y) * texel, receiverDepth);
        return visible / 9.0;
    }

    // Tier 1: PCSS. Search for blockers, estimate penumbra, then filter with
    // a variable-radius Poisson kernel so contact shadows remain sharp and
    // widen naturally with receiver/blocker separation.
    float blockerDepth = 0.0;
    float blockerCount = 0.0;
    vec2 searchRadius = texel * 5.0;
    for (int i = 0; i < 16; ++i) {
        float sampleDepth = texture(uShadowMap, projected.xy + SHADOW_POISSON[i] * searchRadius).r;
        if (sampleDepth < receiverDepth) {
            blockerDepth += sampleDepth;
            blockerCount += 1.0;
        }
    }
    if (blockerCount < 0.5) return 1.0;
    blockerDepth /= blockerCount;
    float penumbra = clamp((receiverDepth - blockerDepth) / max(blockerDepth, 0.0001), 0.0, 0.12);
    vec2 filterRadius = texel * mix(1.5, 11.0, penumbra / 0.12);
    float visible = 0.0;
    for (int i = 0; i < 24; ++i)
        visible += shadow_compare(projected.xy + SHADOW_POISSON[i] * filterRadius, receiverDepth);
    return visible / 24.0;
}

float point_shadow_visibility(int lightIndex, vec3 normal, vec3 lightDirection, float distanceToLight) {
    if (uPointShadowEnabled == 0 || uShadowsEnabled == 0 || uReceivesShadow == 0
        || lightIndex != uPointShadowLightIndex) return 1.0;

    vec3 fromLight = fs.worldPosition - uPointPositionRadius[lightIndex].xyz;
    float slopeBias = mix(0.018, 0.060, 1.0 - max(dot(normal, lightDirection), 0.0));
    float diskRadius = (0.045 + distanceToLight / max(uPointShadowFarPlane, 0.001) * 0.085);
    const vec3 offsets[12] = vec3[](
        vec3( 1, 1, 1), vec3(-1, 1, 1), vec3( 1,-1, 1), vec3(-1,-1, 1),
        vec3( 1, 1,-1), vec3(-1, 1,-1), vec3( 1,-1,-1), vec3(-1,-1,-1),
        vec3( 1, 0, 0), vec3(-1, 0, 0), vec3( 0, 1, 0), vec3( 0,-1, 0)
    );
    int sampleCount = uTier0Profile != 0 ? 8 : 12;
    float visible = 0.0;
    for (int i = 0; i < 12; ++i) {
        if (i >= sampleCount) break;
        float closest = texture(uPointShadowMap, fromLight + offsets[i] * diskRadius).r
                      * uPointShadowFarPlane;
        visible += distanceToLight - slopeBias <= closest ? 1.0 : 0.0;
    }
    return visible / float(sampleCount);
}

vec3 pbr_brdf(vec3 n, vec3 v, vec3 l, vec3 radiance, vec3 albedo, float metallic, float roughness, vec3 f0) {
    vec3 h = normalize(v + l);
    float ndf = distribution_ggx(n, h, roughness);
    float geometry = geometry_smith(n, v, l, roughness);
    vec3 fresnel = fresnel_schlick(max(dot(h, v), 0.0), f0);
    vec3 specular = (ndf * geometry * fresnel) / max(4.0 * max(dot(n, v), 0.0) * max(dot(n, l), 0.0), 0.001);
    vec3 kd = (vec3(1.0) - fresnel) * (1.0 - metallic);
    return (kd * albedo / PI + specular) * radiance * max(dot(n, l), 0.0);
}

vec3 clearcoat_brdf(vec3 n, vec3 v, vec3 l, vec3 radiance, float factor, float roughness) {
    if (factor <= 0.0001) return vec3(0.0);
    vec3 h = normalize(v + l);
    float ndf = distribution_ggx(n, h, max(roughness, 0.04));
    float geometry = geometry_smith(n, v, l, max(roughness, 0.04));
    float fresnel = 0.04 + 0.96 * pow(clamp(1.0 - dot(h, v), 0.0, 1.0), 5.0);
    float specular = ndf * geometry * fresnel / max(4.0 * max(dot(n,v),0.0) * max(dot(n,l),0.0), 0.001);
    return radiance * max(dot(n,l),0.0) * specular * factor;
}

vec3 blinn_phong(vec3 n, vec3 v, vec3 l, vec3 radiance, vec3 albedo, float roughness, float metallic) {
    float diffuse = max(dot(n,l),0.0);
    vec3 h = normalize(v+l);
    float exponent = mix(160.0, 8.0, roughness);
    float specular = pow(max(dot(n,h),0.0), exponent) * mix(0.18,1.0,metallic);
    return (albedo * diffuse + vec3(specular)) * radiance;
}

vec3 shade_light(vec3 n, vec3 v, vec3 l, vec3 radiance, vec3 albedo, float metallic, float roughness, vec3 f0, float coat, float coatRoughness) {
    vec3 base = uShadingMode == 1
        ? blinn_phong(n,v,l,radiance,albedo,roughness,metallic)
        : pbr_brdf(n,v,l,radiance,albedo,metallic,roughness,f0);
    return base + clearcoat_brdf(n,v,l,radiance,coat,coatRoughness);
}

float projected_cookie() {
    if (uProjectedTextureEnabled == 0 || !material_has(FEATURE_PROJECTED_TEXTURE)) return 1.0;
    vec3 projected = fs.projectorPosition.xyz / max(fs.projectorPosition.w, 0.00001);
    projected = projected * 0.5 + 0.5;
    if (projected.z <= 0.0 || projected.z >= 1.0 || any(lessThan(projected.xy,vec2(0.0))) || any(greaterThan(projected.xy,vec2(1.0)))) return 0.0;
    return dot(texture(uProjectedTexture, projected.xy).rgb, vec3(0.333333));
}

float spotlight_cone(vec3 direction, vec3 fragmentToLight,
                     float outerCosine, float innerCosine, int dualSided) {
    vec3 axis = normalize(-direction);
    float cosine = dot(axis, fragmentToLight);
    if (dualSided != 0) cosine = max(cosine, dot(-axis, fragmentToLight));
    return smoothstep(outerCosine, innerCosine, cosine);
}

float spotlight_falloff(float distanceToLight, float range) {
    float normalizedDistance = distanceToLight / max(range, 0.001);
    return pow(clamp(1.0 - pow(normalizedDistance, 4.0), 0.0, 1.0), 2.0)
        / max(distanceToLight * distanceToLight, 0.3);
}

void main() {
    if (uEditorDebugOverride != 0) {
        outScene = uEditorDebugColor;
        outBright = vec4(0.0);
        return;
    }

    vec3 viewDirection = normalize(uCameraPosition - fs.worldPosition);
    vec3 viewTangent = normalize(transpose(fs.tbn) * viewDirection);
    vec2 uv = parallax_uv(fs.uv, viewTangent);

    vec4 albedoSample = texture(uAlbedoMap, uv);
    float opacity = albedoSample.a * texture(uOpacityMap, uv).r;
    if (uAlphaCutoff > 0.0 && opacity < uAlphaCutoff) discard;
    vec3 albedo = albedoSample.rgb * uBaseColorFactor * fs.vertexColor.rgb;
    vec3 orm = texture(uOrmMap, uv).rgb;
    float ao = clamp(orm.r,0.0,1.0);
    float roughness = clamp(orm.g * uRoughnessFactor, 0.045, 1.0);
    float metallic = clamp(orm.b * uMetallicFactor, 0.0, 1.0);
    float coat = (uClearcoatEnabled != 0 && material_has(FEATURE_CLEARCOAT)) ? clamp(uClearcoatFactor * uGlobalClearcoatStrength,0.0,1.0) : 0.0;
    float coatRoughness = clamp(uClearcoatRoughness,0.04,1.0);
    float transmission = (uReflectionRefractionEnabled != 0 && material_has(FEATURE_TRANSMISSION)) ? clamp(uTransmissionFactor*uGlobalTransmissionStrength,0.0,1.0) : 0.0;

    vec3 normal = normalize(fs.tbn[2]);
    if (uNormalMappingEnabled != 0 && material_has(FEATURE_NORMAL_MAPPING)) {
        vec3 tangentNormal = texture(uNormalMap, uv).xyz * 2.0 - 1.0;
        tangentNormal.xy *= uNormalScale * uGlobalNormalStrength;
        normal = normalize(fs.tbn * normalize(tangentNormal));
    }

    vec3 emissive = texture(uEmissiveMap, uv).rgb * uEmissiveFactor;
    if (uUnlit != 0 || uShadingMode == 2) {
        vec3 color = albedo + emissive;
        outScene = vec4(color, opacity);
        outBright = vec4(dot(color,vec3(.2126,.7152,.0722))>1.0?color:vec3(0),opacity);
        return;
    }

    if (uShadingMode == 3) { outScene=vec4(normal*.5+.5,1);outBright=vec4(0,0,0,1);return; }
    if (uShadingMode == 4) { outScene=vec4(fract(uv),0,1);outBright=vec4(0,0,0,1);return; }
    if (uShadingMode == 5) { outScene=vec4(vec3(roughness),1);outBright=vec4(0,0,0,1);return; }
    if (uShadingMode == 6) { outScene=vec4(vec3(metallic),1);outBright=vec4(0,0,0,1);return; }
    if (uShadingMode == 7) { outScene=vec4(vec3(ao),1);outBright=vec4(0,0,0,1);return; }

    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    vec3 color = vec3(0.0);
    float shadow = 1.0;
    vec3 toSun = normalize(-uSunDirection);
    if (uDirectionalEnabled != 0) {
        shadow = shadow_visibility(normal,toSun);
        color += shade_light(normal,viewDirection,toSun,uSunRadiance*uSunIntensity,albedo,metallic,roughness,f0,coat,coatRoughness)*shadow;
    }
    if (uShadingMode == 8) { outScene=vec4(vec3(shadow),1);outBright=vec4(0,0,0,1);return; }

    if (uPointLightsEnabled != 0) {
        for (int i = 0; i < uPointLightCount; ++i) {
            vec3 delta = uPointPositionRadius[i].xyz - fs.worldPosition;
            float distanceToLight = length(delta);
            float radius = uPointPositionRadius[i].w;
            vec3 l = delta / max(distanceToLight, 0.0001);
            float normalizedDistance = distanceToLight / max(radius,0.001);
            float falloff = pow(clamp(1.0-pow(normalizedDistance,4.0),0.0,1.0),2.0)/max(distanceToLight*distanceToLight,0.3);
            float localShadow = point_shadow_visibility(i, normal, l, distanceToLight);
            vec3 radiance = uPointColorIntensity[i].rgb * uPointColorIntensity[i].a * falloff * localShadow;
            color += shade_light(normal,viewDirection,l,radiance,albedo,metallic,roughness,f0,coat,coatRoughness);
        }
    }

    if (uSpotEnabled != 0) {
        vec3 delta = uSpotPositionRange.xyz - fs.worldPosition;
        float distanceToLight = length(delta);
        vec3 l = delta / max(distanceToLight, 0.0001);
        float cone = spotlight_cone(uSpotDirectionOuter.xyz, l,
            uSpotDirectionOuter.w, uSpotInnerCosine, uSpotDualSided);
        float falloff = spotlight_falloff(distanceToLight, uSpotPositionRange.w);
        vec3 radiance = uSpotColorIntensity.rgb * uSpotColorIntensity.a * falloff * cone;
        color += shade_light(normal, viewDirection, l, radiance, albedo, metallic,
            roughness, f0, coat, coatRoughness);
    }

    if (uSpot2Enabled != 0) {
        vec3 delta = uSpot2PositionRange.xyz - fs.worldPosition;
        float distanceToLight = length(delta);
        vec3 l = delta / max(distanceToLight, 0.0001);
        float cone = spotlight_cone(uSpot2DirectionOuter.xyz, l,
            uSpot2DirectionOuter.w, uSpot2InnerCosine, uSpot2DualSided);
        float falloff = spotlight_falloff(distanceToLight, uSpot2PositionRange.w);
        vec3 radiance = uSpot2ColorIntensity.rgb * uSpot2ColorIntensity.a * falloff * cone;
        color += shade_light(normal, viewDirection, l, radiance, albedo, metallic,
            roughness, f0, coat, coatRoughness);
    }


    if (uSpot3Enabled != 0) {
        vec3 delta = uSpot3PositionRange.xyz - fs.worldPosition;
        float distanceToLight = length(delta);
        vec3 l = delta / max(distanceToLight, 0.0001);
        float cone = spotlight_cone(uSpot3DirectionOuter.xyz, l,
            uSpot3DirectionOuter.w, uSpot3InnerCosine, uSpot3DualSided);
        float falloff = spotlight_falloff(distanceToLight, uSpot3PositionRange.w);
        float cookie = mix(1.0, projected_cookie(), clamp(uProjectorStrength, 0.0, 1.0));
        vec3 radiance = uSpot3ColorIntensity.rgb * uSpot3ColorIntensity.a * falloff * cone * cookie;
        color += shade_light(normal, viewDirection, l, radiance, albedo, metallic,
            roughness, f0, coat, coatRoughness);
    }

    if (uEnvironmentEnabled != 0 && material_has(FEATURE_ENVIRONMENT)) {
        vec3 reflection = reflect(-viewDirection,normal);
        vec3 fAmbient = fresnel_roughness(max(dot(normal,viewDirection),0.0),f0,roughness);
        vec3 kdAmbient = (1.0-fAmbient)*(1.0-metallic);
        vec3 diffuseIbl = textureLod(uEnvironmentMap,normal,5.0).rgb*albedo;
        vec3 specularIbl = textureLod(uEnvironmentMap,reflection,roughness*5.0).rgb*fAmbient;
        vec3 environment = (kdAmbient*diffuseIbl+specularIbl)*ao*uEnvironmentStrength;
        if (coat > 0.0) {
            vec3 coatReflection = textureLod(uEnvironmentMap, reflection, coatRoughness*5.0).rgb;
            float coatFresnel = 0.04 + 0.96*pow(1.0-max(dot(normal,viewDirection),0.0),5.0);
            environment += coatReflection*coatFresnel*coat*uEnvironmentStrength;
        }
        if (transmission > 0.0) {
            float eta = 1.0/max(uIndexOfRefraction,1.001);
            vec3 refracted = refract(-viewDirection,normal,eta);
            vec3 transmitted = textureLod(uEnvironmentMap,refracted,roughness*4.0).rgb;
            environment = mix(environment, transmitted*uBaseColorFactor, transmission);
        }
        color += environment;
    } else {
        color += albedo * ao * 0.025;
    }

    if (uRimEnabled != 0 && material_has(FEATURE_RIM)) {
        float rim = pow(1.0 - max(dot(normal, viewDirection), 0.0), 3.4);
        color += vec3(0.22, 0.30, 0.40) * rim * 0.22;
    }
    color += emissive;
    if (uToonEnabled != 0 && material_has(FEATURE_TOON)) {
        vec3 quantized = floor(color * 9.0 + 0.5) / 9.0;
        color = mix(color, quantized, 0.48);
    }
    if (uFogEnabled != 0) {
        float distanceToCamera = length(uCameraPosition - fs.worldPosition);
        float fog = fog_amount(distanceToCamera, fs.worldPosition.y);
        color = mix(color, fog_color(viewDirection), fog);
    }

    float finalOpacity = mix(opacity, opacity * 0.42, transmission);
    outScene = vec4(color, finalOpacity);
    float brightness=dot(color,vec3(0.2126,0.7152,0.0722));
    outBright=vec4(brightness>1.15?color:vec3(0.0),opacity);
}
