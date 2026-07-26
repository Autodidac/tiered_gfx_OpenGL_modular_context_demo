#version 450 core
layout(location = 0) out vec4 outColor;
in vec2 uv;
uniform sampler2D uImage;
uniform sampler2D uBrightImage;
uniform int uHorizontal;
uniform int uPrefilter;
uniform float uThreshold;

vec3 threshold_extract(vec3 color) {
    float brightness = max(max(color.r, color.g), color.b);
    float knee = max(uThreshold * 0.50, 0.0001);
    float soft = clamp(brightness - uThreshold + knee, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 0.0001);
    float contribution = max(brightness - uThreshold, soft) / max(brightness, 0.0001);
    return color * contribution;
}

vec3 sample_source(vec2 coordinate) {
    vec3 source = texture(uImage, coordinate).rgb;
    if (uPrefilter == 0) return source;

    // Preserve explicit emissive/bright MRT output while also allowing naturally
    // bright HDR surfaces to bloom. This avoids a black bloom chain when a shader
    // intentionally writes only one of the two sources.
    vec3 authoredBright = texture(uBrightImage, coordinate).rgb;
    return max(threshold_extract(source), authoredBright);
}

void main() {
    const float weights[5] = float[5](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
    vec2 texel = 1.0 / vec2(textureSize(uImage, 0));
    vec3 result = sample_source(uv) * weights[0];
    for (int i = 1; i < 5; ++i) {
        vec2 offset = uHorizontal != 0 ? vec2(texel.x * float(i), 0.0)
                                      : vec2(0.0, texel.y * float(i));
        result += sample_source(uv + offset) * weights[i];
        result += sample_source(uv - offset) * weights[i];
    }
    outColor = vec4(result, 1.0);
}
