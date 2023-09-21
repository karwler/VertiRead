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

void main() {
	vec2 dpos, dsiz = vec2(0.0);
	if (pc.rect[2] > 0 && pc.rect[3] > 0 && pc.frame[2] > 0 && pc.frame[3] > 0) {
		dpos = vec2(max(pc.rect.xy, pc.frame.xy));
		dsiz = vec2(min(pc.rect.xy + pc.rect.zw, pc.frame.xy + pc.frame.zw)) - dpos;
	}

	if (dsiz.x > 0.0 && dsiz.y > 0.0) {
		vec2 loc = vposs[gl_VertexIndex] * dsiz + dpos;
		gl_Position = vec4((loc - ud.pview.xy) / ud.pview.zw - 1.0, 0.0, 1.0);
   } else
		gl_Position = vec4(-2.0, -2.0, 0.0, 1.0);
}
