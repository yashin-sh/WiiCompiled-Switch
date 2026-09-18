#version 450

layout(location = 0) out vec3 v_color;

const vec2 positions[3] = vec2[](
    vec2( 0.0, -0.72),
    vec2( 0.72, 0.62),
    vec2(-0.72, 0.62)
);

const vec3 colors[3] = vec3[](
    vec3(1.0, 0.15, 0.12),
    vec3(0.10, 1.0, 0.25),
    vec3(0.15, 0.35, 1.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    v_color = colors[gl_VertexIndex];
}
