#version 450 core
layout(vertices=3) out;
in VS_OUT { vec3 position; vec3 normal; vec2 uv; } vin[];
out TC_OUT { vec3 position; vec3 normal; vec2 uv; } vout[];
uniform mat4 uModel;
uniform vec3 uCameraPosition;
uniform float uTessLevel;

float edge_level(vec3 a, vec3 b) {
    vec3 midpoint = (uModel * vec4((a + b) * 0.5, 1.0)).xyz;
    float distanceToCamera = length(midpoint - uCameraPosition);
    float proximity = 1.0 - smoothstep(10.0, 58.0, distanceToCamera);
    return mix(1.0, clamp(uTessLevel, 1.0, 32.0), proximity);
}

void main() {
    vout[gl_InvocationID].position = vin[gl_InvocationID].position;
    vout[gl_InvocationID].normal = vin[gl_InvocationID].normal;
    vout[gl_InvocationID].uv = vin[gl_InvocationID].uv;
    if (gl_InvocationID == 0) {
        gl_TessLevelOuter[0] = edge_level(vin[1].position, vin[2].position);
        gl_TessLevelOuter[1] = edge_level(vin[2].position, vin[0].position);
        gl_TessLevelOuter[2] = edge_level(vin[0].position, vin[1].position);
        gl_TessLevelInner[0] = (gl_TessLevelOuter[0] + gl_TessLevelOuter[1]
                              + gl_TessLevelOuter[2]) / 3.0;
    }
}
