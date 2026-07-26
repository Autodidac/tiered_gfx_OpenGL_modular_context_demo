#version 450 core
layout(location=0) out vec4 outScene;layout(location=1) out vec4 outBright;in vec2 uv;uniform float uTime;
float grid(vec2 p){vec2 q=abs(fract(p)-.5)/fwidth(p);return 1.0-min(min(q.x,q.y),1.0);}
void main(){vec2 p=uv*2.0-1.0;float ring=smoothstep(.04,0.0,abs(length(p)-(.45+.05*sin(uTime))));float g=grid(uv*12.0)*.22;vec3 color=vec3(.015,.035,.07)+vec3(.05,.42,1.5)*(ring+g)+vec3(1.4,.18,.04)*smoothstep(.08,0.0,length(p-vec2(sin(uTime)*.35,cos(uTime*.7)*.25)));outScene=vec4(color,1);outBright=vec4(max(color-vec3(.8),0.0),1);}
