#version 450

layout(set = 0, binding = 1) uniform sampler2D fontSampler[26];

layout(location = 0) in struct {
    vec3 color;
    vec2 texCoord;
} outValue;

flat layout(location = 2) in uint index;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(outValue.color, 0.0);
    outColor.a = texture(fontSampler[index], outValue.texCoord).r;
}