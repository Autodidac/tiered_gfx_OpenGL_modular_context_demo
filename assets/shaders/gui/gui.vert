#version 450 core
layout(location=0) in vec2 aPosition;
layout(location=1) in vec2 aUv;
layout(location=2) in vec4 aColor;
layout(location=3) in float aTextured;
uniform vec2 uViewport;
out vec2 vUv;
out vec4 vColor;
out float vTextured;
void main(){
    vec2 ndc=vec2(aPosition.x/uViewport.x*2.0-1.0,1.0-aPosition.y/uViewport.y*2.0);
    gl_Position=vec4(ndc,0.0,1.0);
    vUv=aUv;vColor=aColor;vTextured=aTextured;
}
