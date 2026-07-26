#version 450 core
layout(location=0) in vec3 aPosition;
layout(location=2) in vec2 aUv;

out TE_OUT { vec3 world; vec3 normal; vec2 uv; vec4 lightPosition; } v;

uniform mat4 uModel;
uniform mat4 uViewProjection;
uniform mat4 uLightViewProjection;
uniform sampler2D uHeightMap;
uniform float uHeightScale;

float terrain_height(vec2 uv) {
    vec2 clamped = clamp(uv, 0.0, 1.0);
    vec2 worldXZ = (clamped - 0.5) * 64.0 + vec2(0.0, -3.0);
    float height = (texture(uHeightMap, clamped).r - 0.42) * uHeightScale;

    vec2 hall = abs((worldXZ - vec2(0.0, -14.0)) / vec2(17.0, 7.2));
    float hallFlat = 1.0 - smoothstep(0.88, 1.08, max(hall.x, hall.y));
    vec2 plaza = abs((worldXZ - vec2(0.0, 3.0)) / vec2(19.0, 11.0));
    float plazaFlat = 1.0 - smoothstep(0.86, 1.12, max(plaza.x, plaza.y));
    height = mix(height, 0.0, max(hallFlat, plazaFlat));

    vec2 pool = abs((worldXZ - vec2(16.0, 5.0)) / vec2(6.2, 4.0));
    float poolCut = 1.0 - smoothstep(0.82, 1.08, max(pool.x, pool.y));
    height = mix(height, -0.38, poolCut);

    vec2 lake = (worldXZ - vec2(0.0, -30.0)) / vec2(25.0, 8.0);
    float lakeCut = 1.0 - smoothstep(0.76, 1.02, length(lake));
    height = mix(height, -0.72, lakeCut);
    return height;
}

void main() {
    vec3 p = aPosition;
    vec2 uv = aUv;
    p.y += terrain_height(uv);
    float e = 1.0 / 1024.0;
    float hx = terrain_height(uv + vec2(e,0.0)) - terrain_height(uv - vec2(e,0.0));
    float hz = terrain_height(uv + vec2(0.0,e)) - terrain_height(uv - vec2(0.0,e));
    vec3 localNormal = normalize(vec3(-hx, 2.0 * e * 64.0, -hz));
    vec4 world = uModel * vec4(p, 1.0);
    mat3 normalMatrix = transpose(inverse(mat3(uModel)));
    v.world = world.xyz;
    v.normal = normalize(normalMatrix * localNormal);
    v.uv = uv;
    v.lightPosition = uLightViewProjection * world;
    gl_Position = uViewProjection * world;
}
