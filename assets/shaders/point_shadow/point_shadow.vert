#version 450 core
layout(location=0) in vec3 aPosition;
uniform mat4 uModel;
uniform mat4 uLightViewProjection;
out vec3 vWorldPosition;
void main() {
    vec4 world = uModel * vec4(aPosition, 1.0);
    vWorldPosition = world.xyz;
    gl_Position = uLightViewProjection * world;
}
