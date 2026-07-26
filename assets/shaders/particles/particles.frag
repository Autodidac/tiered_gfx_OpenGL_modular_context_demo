#version 450 core
layout(location=0) out vec4 outScene;
layout(location=1) out vec4 outBright;
in vec4 vColor;
void main(){vec2 p=gl_PointCoord*2.0-1.0;float r=dot(p,p);if(r>1.0)discard;float alpha=(1.0-r)*vColor.a;vec3 color=vColor.rgb*(1.2+2.2*(1.0-r));outScene=vec4(color,alpha);outBright=vec4(color*alpha,alpha);}
