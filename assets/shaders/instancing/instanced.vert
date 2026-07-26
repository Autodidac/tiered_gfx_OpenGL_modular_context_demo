#version 450 core
layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUv;

uniform mat4 uViewProjection;
uniform mat4 uInstanceModels[12];

out vec3 vNormal;
out vec3 vWorld;
out vec2 vUv;

void main() {
    mat4 model = uInstanceModels[gl_InstanceID];
    vec4 world = model * vec4(aPosition, 1.0);
    vWorld = world.xyz;
    vNormal = normalize(mat3(model) * aNormal);
    vUv = aUv;
    gl_Position = uViewProjection * world;
}
