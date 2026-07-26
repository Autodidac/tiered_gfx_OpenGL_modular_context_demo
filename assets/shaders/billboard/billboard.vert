#version 450 core
layout(location=0) in vec2 aLocal;
layout(location=1) in vec2 aUv;
layout(location=2) in vec3 aCenter;
layout(location=3) in vec2 aSize;
layout(location=4) in float aPhase;
layout(location=5) in float aStiffness;

uniform mat4 uViewProjection;
uniform vec2 uBladeScale;
uniform float uSwayScale;
uniform vec3 uCameraRight;
uniform float uTime;
uniform float uCrossAngle;

out vec2 vUv;
out float vHeight;
out float vShade;

void main() {
    vec3 cameraRight = normalize(vec3(uCameraRight.x, 0.0, uCameraRight.z));
    vec3 perpendicular = vec3(-cameraRight.z, 0.0, cameraRight.x);
    vec3 right = normalize(cameraRight * cos(uCrossAngle) + perpendicular * sin(uCrossAngle));
    vec3 up = vec3(0.0, 1.0, 0.0);
    float height = aLocal.y;
    float gust = sin(uTime * 1.72 + aPhase) * 0.095
               + sin(uTime * 0.61 + aPhase * 1.71) * 0.050
               + sin(uTime * 3.25 + aPhase * 0.43) * 0.018;
    vec3 windForward = normalize(vec3(0.62, 0.0, 0.78));
    vec3 bend = (right * gust + windForward * gust * 0.34)
              * height * height / max(aStiffness, 0.25) * uSwayScale;
    vec3 center = aCenter;
    vec2 size = aSize * uBladeScale;
    vec3 world = center + right * (aLocal.x * size.x) + up * (height * size.y) + bend;
    gl_Position = uViewProjection * vec4(world, 1.0);
    vUv = aUv;
    vHeight = height;
    vShade = 0.88 + 0.12 * sin(aPhase * 2.0);
}
