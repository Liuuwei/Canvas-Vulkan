#version 450

layout(set = 0, binding = 1) uniform sampler2D texSampler; 

layout(location = 0) in vec3 inColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(inColor, 1.0);
}