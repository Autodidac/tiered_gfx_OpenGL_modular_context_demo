#version 450 core
layout(location=0) out vec4 outScene;
layout(location=1) out vec4 outBright;

in vec2 vUv;
uniform sampler2D uScreen;
uniform int uFeedMode; // 0 security, 1 overhead site camera, 2 ceiling camera, 3 planar mirror
uniform float uTime;

float line_segment(vec2 p, vec2 a, vec2 b, float width) {
    vec2 pa = p - a;
    vec2 ba = b - a;
    float h = clamp(dot(pa, ba) / max(dot(ba, ba), 0.00001), 0.0, 1.0);
    return 1.0 - smoothstep(width, width + 0.002, length(pa - ba * h));
}

float frame_border(vec2 coordinate) {
    vec2 inside = smoothstep(vec2(0.0), vec2(0.014), coordinate)
                * smoothstep(vec2(0.0), vec2(0.014), 1.0 - coordinate);
    return inside.x * inside.y;
}

void main() {
    vec2 sampleUv = clamp(vUv, vec2(0.001), vec2(0.999));

    // Keep the feed in linear HDR. The main post-process pass performs the only
    // tone-map and gamma conversion, preventing the old double-tonemapped dark feed.
    vec3 color = max(texture(uScreen, sampleUv).rgb, vec3(0.0));
    float border = frame_border(vUv);
    color *= border;

    if (uFeedMode == 0 || uFeedMode == 2) {
        float scan = 0.975 + 0.025 * sin(vUv.y * 760.0 + uTime * 1.45);
        float vignette = smoothstep(0.84, 0.20, length(vUv - 0.5));
        color *= scan * mix(0.82, 1.0, vignette);
        color = mix(color, color * vec3(0.88, 1.02, 1.08), 0.18);

        float reticle = line_segment(vUv, vec2(0.465,0.50), vec2(0.535,0.50), 0.0012)
                      + line_segment(vUv, vec2(0.50,0.455), vec2(0.50,0.545), 0.0012);
        float recording = 1.0 - smoothstep(0.014, 0.019, length(vUv - vec2(0.075, 0.065)));
        recording *= 0.60 + 0.40 * step(0.0, sin(uTime * 5.0));
        color += vec3(0.10, 0.55, 0.80) * clamp(reticle, 0.0, 1.0) * 0.42;
        color += vec3(2.2, 0.03, 0.015) * recording;
    } else if (uFeedMode == 1) {
        vec2 gridUv = vUv * vec2(18.0, 10.0);
        vec2 cell = abs(fract(gridUv - 0.5) - 0.5) / max(fwidth(gridUv), vec2(0.0001));
        float grid = 1.0 - min(min(cell.x, cell.y), 1.0);
        float sweepY = fract(uTime * 0.075);
        float sweep = 1.0 - smoothstep(0.0, 0.018, abs(vUv.y - sweepY));
        color = mix(color, color * vec3(0.78, 1.0, 0.86), 0.16);
        color += vec3(0.04, 0.30, 0.14) * grid * 0.20;
        color += vec3(0.10, 1.1, 0.40) * sweep * 0.30;
    } else {
        // A reflected camera already creates the mirror reversal. Do not flip U again.
        float silverEdge = (1.0 - border) * 0.22;
        color += vec3(0.12, 0.15, 0.18) * silverEdge;
    }

    float brightness = max(max(color.r, color.g), color.b);
    outScene = vec4(color, 1.0);
    outBright = vec4(brightness > 0.95 ? color : vec3(0.0), 1.0);
}
