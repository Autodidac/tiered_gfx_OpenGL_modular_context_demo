#version 450 core
layout(location=0) in vec3 aPosition;
layout(location=1) in vec4 aColor;
layout(location=2) in float aSize;
uniform mat4 uViewProjection;
out vec4 vColor;
void main(){gl_Position=uViewProjection*vec4(aPosition,1.0);gl_PointSize=aSize/max(gl_Position.w,0.15);vColor=aColor;}
