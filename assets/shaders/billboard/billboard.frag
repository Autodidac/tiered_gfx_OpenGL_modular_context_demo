#version 450 core
layout(location=0) out vec4 outScene;
layout(location=1) out vec4 outBright;

in vec2 vUv;
in float vHeight;
in float vShade;

uniform sampler2D uAlbedo;
uniform sampler2D uOpacity;
uniform vec3 uSunDirection;
uniform vec3 uSunColor;

void main() {
    vec4 sampleColor = texture(uAlbedo, vUv);
    float alpha = sampleColor.a * texture(uOpacity, vUv).r;
    if (alpha < 0.18) discard;

    float topLight = mix(0.72, 1.0, vHeight);
    float sun = clamp(-uSunDirection.y, 0.15, 1.0);
    vec3 color = sampleColor.rgb * vShade * topLight * (0.42 + uSunColor * sun * 0.38);
    outScene = vec4(color, 1.0);
    outBright = vec4(0.0, 0.0, 0.0, 1.0);
}
