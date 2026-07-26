#version 450 core
layout(location = 0) out vec4 outScene;
layout(location = 1) out vec4 outBright;
in vec3 direction;
uniform samplerCube uEnvironment;
uniform float uExposureScale;
uniform vec3 uSunColor;
uniform vec3 uSunDirection;

float hash31(vec3 p) {
    p = fract(p * 0.1031);
    p += dot(p, p.yzx + 33.33);
    return fract((p.x + p.y) * p.z);
}

void main() {
    vec3 dir = normalize(direction);
    float daylight = smoothstep(-0.08, 0.34, -uSunDirection.y);
    float horizon = pow(1.0 - abs(dir.y), 3.0);
    vec3 sunDir = normalize(-uSunDirection);
    float sunAlignment = max(dot(dir, sunDir), 0.0);

    // Never cut a dark hole out of the environment around the moving sun. The
    // former broad subtraction mask was larger than the disc and produced the
    // visible black corona. The sun is now purely additive over the sky.
    vec3 environment = textureLod(uEnvironment, dir, 0.0).rgb;
    vec3 night = vec3(0.007, 0.012, 0.030) + environment * 0.035;
    vec3 day = environment * uExposureScale;
    vec3 color = mix(night, day, daylight);
    color += uSunColor * horizon * (1.0 - daylight) * 0.055;

    float sunDisc = smoothstep(0.99982, 0.99994, sunAlignment);
    float innerGlow = pow(sunAlignment, 96.0);
    float outerGlow = pow(sunAlignment, 18.0);
    color += uSunColor * daylight
        * (sunDisc * 1.65 + innerGlow * 0.16 + outerGlow * 0.018);

    float starCell = hash31(floor(dir * 780.0));
    float stars = step(0.9975, starCell) * pow(max(abs(dir.y), 0.0), 0.35) * (1.0 - daylight);
    color += vec3(0.65, 0.75, 1.0) * stars;

    outScene = vec4(color, 1.0);
    float brightness = dot(color, vec3(0.2126,0.7152,0.0722));
    outBright = vec4(brightness > 1.2 ? color : vec3(0.0), 1.0);
}
