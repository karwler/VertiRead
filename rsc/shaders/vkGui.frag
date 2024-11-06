#version 460

layout(push_constant) uniform PushData {
	ivec4 rect;
	ivec4 frame;
	uint color;
	uint sid;
} pc;

layout(set = 0, binding = 0) uniform GlobalData {
	vec4 colors[8];
	float gamma;
} gd;

layout(set = 0, binding = 1) uniform sampler colorSamp[2];
layout(set = 2, binding = 0) uniform texture2D colorTex;

layout(location = 0) noperspective in vec2 fragUV;

layout(location = 0) out vec4 outColor;

void main() {
	outColor = texture(sampler2D(colorTex, colorSamp[pc.sid]), fragUV) * gd.colors[pc.color];
}
