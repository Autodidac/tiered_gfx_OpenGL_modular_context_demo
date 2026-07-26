#version 450 core
layout(location=0) out vec4 outColor;
in vec2 vUv;
in vec4 vColor;
in float vTextured;
uniform sampler2D uFont;
void main(){
    float sampled = texture(uFont, vUv).a;
    float glyph = smoothstep(0.02, 0.98, sampled);
    float alpha = mix(1.0, glyph, clamp(vTextured, 0.0, 1.0));
    outColor = vec4(vColor.rgb, vColor.a * alpha);
}
