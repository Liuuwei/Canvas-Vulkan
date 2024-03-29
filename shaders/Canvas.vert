#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out struct {
    vec3 color;
    vec2 texCoord;
} outValue;

void main() {
    gl_Position =  ubo.proj * vec4(inPosition, 0.0, 1.0);
    outValue.color = inColor;
    outValue.texCoord = inTexCoord;
}