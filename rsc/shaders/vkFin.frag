#version 460

layout(set = 0, binding = 0) uniform GlobalData {
	vec4 colors[8];
	float gamma;
} gd;

layout(input_attachment_index = 0, set = 1, binding = 1) uniform subpassInput inColor;

layout(location = 0) out vec4 outColor;

void main() {
	outColor = vec4(pow(subpassLoad(inColor).rgb, vec3(gd.gamma)), 1.0);
}
