#version 450 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUv;
layout(location = 3) in vec4 aTangent;
layout(location = 4) in vec4 aColor;

uniform mat4 uModel;
uniform mat4 uViewProjection;
uniform mat4 uLightViewProjection;
uniform mat4 uProjectorViewProjection;
uniform float uUvScale;

out VS_OUT {
    vec3 worldPosition;
    vec2 uv;
    mat3 tbn;
    vec4 lightPosition;
    vec4 projectorPosition;
    vec4 vertexColor;
} vs;

void main() {
    vec4 world = uModel * vec4(aPosition, 1.0);
    mat3 normalMatrix = transpose(inverse(mat3(uModel)));
    vec3 n = normalize(normalMatrix * aNormal);
    vec3 t = normalize(mat3(uModel) * aTangent.xyz);
    t = normalize(t - n * dot(n, t));
    vec3 b = normalize(cross(n, t)) * aTangent.w;
    vs.worldPosition = world.xyz;
    vs.uv = aUv * uUvScale;
    vs.tbn = mat3(t, b, n);
    vs.lightPosition = uLightViewProjection * world;
    vs.projectorPosition = uProjectorViewProjection * world;
    vs.vertexColor = aColor;
    gl_Position = uViewProjection * world;
}
