#version 450 core
in vec3 vWorldPosition;
uniform vec3 uLightPosition;
uniform float uFarPlane;
void main() {
    gl_FragDepth = length(vWorldPosition - uLightPosition) / max(uFarPlane, 0.001);
}
