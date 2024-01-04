#version 450

layout(push_constant) uniform PushData {
	ivec4 rect;
	ivec4 frame;
	vec4 color;
	uint sid;
} pc;

layout(set = 0, binding = 0) uniform UniformData {
	vec4 pview;
} u1;

layout(location = 0) in vec2 vpos;

layout(location = 0) noperspective out vec2 fragUV;

void main() {
	vec4 dst = vec4(0.0);
	if (pc.rect[2] > 0 && pc.rect[3] > 0 && pc.frame[2] > 0 && pc.frame[3] > 0) {
		dst.xy = vec2(max(pc.rect.xy, pc.frame.xy));
		dst.zw = vec2(min(pc.rect.xy + pc.rect.zw, pc.frame.xy + pc.frame.zw)) - dst.xy;
	}

	if (dst[2] > 0.0 && dst[3] > 0.0) {
		vec4 uvrc = vec4(dst.xy - vec2(pc.rect.xy), dst.zw) / vec4(pc.rect.zwzw);
		fragUV = vpos * uvrc.zw + uvrc.xy;
		vec2 loc = vpos * dst.zw + dst.xy;
		gl_Position = vec4((loc - u1.pview.xy) / u1.pview.zw - 1.0, 0.0, 1.0);
	} else {
		fragUV = vec2(0.0);
		gl_Position = vec4(-2.0, -2.0, 0.0, 1.0);
	}
}
