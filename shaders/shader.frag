#version 450

layout(set = 0, binding = 1) uniform samplerCube texSampler;

layout(location = 0) in vec3 outTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    // outColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    // outColor = vec4(outTexCoord + 0.1f, 1.0f);
    outColor = texture(texSampler, outTexCoord);
}