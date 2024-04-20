#version 450

layout(binding = 0) uniform UniformBuffer {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec2 resolution;
    uint time;
} ubo;

layout (location = 0) out vec3 fragColor;
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;

/*
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;
*/

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(0.5, 0.5, 0.0),
    vec3(0.0, 0.5, 0.5),
    vec3(0.5, 0.0, 0.5)
);

void main() {
    //gl_Position=ubo.proj*ubo.view*ubo.model*vec4(positions[gl_VertexIndex],0.0,1.0);
    gl_Position = vec4(inPosition, 1.0);
    fragColor = colors[gl_VertexIndex];
}
