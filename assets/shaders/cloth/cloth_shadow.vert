#version 450 core
layout(location=0) in vec3 aPosition;
uniform mat4 uLightViewProjection;
void main() { gl_Position = uLightViewProjection * vec4(aPosition, 1.0); }
