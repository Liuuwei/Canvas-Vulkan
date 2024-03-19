#version 450

layout(set = 0, binding = 1) uniform sampler2D texSampler; 

layout(location = 0) in struct {
    vec3 color;
} outValue;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(outValue.color, 1.0);
}