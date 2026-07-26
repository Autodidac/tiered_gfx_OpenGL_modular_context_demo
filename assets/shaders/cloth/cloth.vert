#version 450 core
layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUv;
layout(location=3) in vec4 aTangent;

uniform mat4 uViewProjection;
uniform mat4 uLightViewProjection;

out VS_OUT {
    vec3 worldPosition;
    vec2 uv;
    mat3 tbn;
    vec4 lightPosition;
} vs;

void main() {
    vec3 normal = normalize(aNormal);
    vec3 tangent = normalize(aTangent.xyz - normal * dot(normal, aTangent.xyz));
    vec3 bitangent = normalize(cross(normal, tangent)) * aTangent.w;
    vs.worldPosition = aPosition;
    vs.uv = aUv;
    vs.tbn = mat3(tangent, bitangent, normal);
    vs.lightPosition = uLightViewProjection * vec4(aPosition, 1.0);
    gl_Position = uViewProjection * vec4(aPosition, 1.0);
}
