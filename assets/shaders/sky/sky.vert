#version 450 core
layout(location = 0) in vec3 aPosition;
uniform mat4 uView;
uniform mat4 uProjection;
out vec3 direction;
void main() {
    mat4 rotationView = mat4(mat3(uView));
    vec4 position = uProjection * rotationView * vec4(aPosition * 60.0, 1.0);
    gl_Position = position.xyww;
    direction = aPosition;
}
