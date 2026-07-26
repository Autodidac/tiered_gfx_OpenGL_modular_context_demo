#version 450 core
layout(location=0) out vec4 outScene;
layout(location=1) out vec4 outBright;
in vec3 vNormal;in vec3 vWorld;in vec2 vUv;
uniform sampler2D uAlbedo;uniform sampler2D uOrm;uniform samplerCube uEnvironment;uniform vec3 uCamera;uniform vec3 uSunDirection;uniform vec3 uSunColor;
void main(){vec3 albedo=texture(uAlbedo,vUv).rgb;vec3 orm=texture(uOrm,vUv).rgb;vec3 n=normalize(vNormal),v=normalize(uCamera-vWorld),l=normalize(-uSunDirection);float ndl=max(dot(n,l),0.0);vec3 r=reflect(-v,n);vec3 env=textureLod(uEnvironment,r,orm.g*5.0).rgb;vec3 color=albedo*(0.12+ndl*uSunColor*.45)+env*.12;outScene=vec4(color,1);outBright=vec4(0,0,0,1);}
