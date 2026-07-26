#version 450 core
layout(location=0) in vec3 aPosition;layout(location=2) in vec2 aUv;uniform mat4 uModel;uniform mat4 uViewProjection;out vec2 vUv;
void main(){gl_Position=uViewProjection*uModel*vec4(aPosition,1);vUv=aUv;}
