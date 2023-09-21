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
	vec4 color;
	uint sid;
} pc;

layout(set = 0, binding = 0) uniform UniformData {
	vec4 pview;
} u1;

layout(location = 0) noperspective out vec2 fragUV;

void main() {
	vec2 dpos, dsiz = vec2(0.0);
	if (pc.rect[2] > 0 && pc.rect[3] > 0 && pc.frame[2] > 0 && pc.frame[3] > 0) {
		dpos = vec2(max(pc.rect.xy, pc.frame.xy));
		dsiz = vec2(min(pc.rect.xy + pc.rect.zw, pc.frame.xy + pc.frame.zw)) - dpos;
	}

	if (dsiz.x > 0.0 && dsiz.y > 0.0) {
		vec2 uvpos = (dpos - vec2(pc.rect.xy)) / vec2(pc.rect.zw);
		vec2 uvsiz = dsiz / vec2(pc.rect.zw);
		fragUV = vposs[gl_VertexIndex] * uvsiz + uvpos;
		vec2 loc = vposs[gl_VertexIndex] * dsiz + dpos;
		gl_Position = vec4((loc - u1.pview.xy) / u1.pview.zw - 1.0, 0.0, 1.0);
	} else {
		fragUV = vec2(0.0);
		gl_Position = vec4(-2.0, -2.0, 0.0, 1.0);
	}
}
