#version 450

const vec2 vposs[4] = vec2[](
	vec2(0.0, 0.0),
	vec2(1.0, 0.0),
	vec2(0.0, 1.0),
	vec2(1.0, 1.0)
);

layout(push_constant) uniform PushData {
	ivec4 rect;
	ivec4 frame;
	uvec2 addr;
} pc;

layout(binding = 0) uniform UniformData {
	vec4 pview;
} ud;

layout(location = 0) in vec2 vpos;

void main() {
	vec4 dst = vec4(0.0);
	if (pc.rect[2] > 0 && pc.rect[3] > 0 && pc.frame[2] > 0 && pc.frame[3] > 0) {
		dst.xy = vec2(max(pc.rect.xy, pc.frame.xy));
		dst.zw = vec2(min(pc.rect.xy + pc.rect.zw, pc.frame.xy + pc.frame.zw)) - dst.xy;
	}

	if (dst[2] > 0.0 && dst[3] > 0.0) {
		vec2 loc = vpos * dst.zw + dst.xy;
		gl_Position = vec4((loc - ud.pview.xy) / ud.pview.zw - 1.0, 0.0, 1.0);
	} else
		gl_Position = vec4(-2.0, -2.0, 0.0, 1.0);
}
