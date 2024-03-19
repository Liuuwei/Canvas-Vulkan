#version 450

layout(set = 0, binding = 1) uniform sampler2D texSampler; 

layout(location = 0) in struct {
    vec3 color;
    vec2 texCoord;
} outValue;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler, outValue.texCoord);
    outColor -= vec4(0.1, 0.1, 0.1, 0.0);
}