#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 outColor;

void main() {
    // mat4 view = mat4(mat3(ubo.view));
    // gl_Position = ubo.proj * view * ubo.model * vec4(inPosition, 1.0f);
    // gl_Position.z = gl_Position.w;
    // outTexCoord = inTexCoord;
    gl_Position =  ubo.proj * vec4(inPosition, 1.0);
    outColor = inColor;
}