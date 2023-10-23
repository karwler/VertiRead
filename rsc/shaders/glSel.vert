#version 130

const vec2 vposs[4] = vec2[](
	vec2(0.0, 0.0),
	vec2(1.0, 0.0),
	vec2(0.0, 1.0),
	vec2(1.0, 1.0)
);

uniform vec4 pview;
uniform ivec4 rect;
uniform ivec4 frame;

void main() {
	vec4 dst = vec4(0.0);
	if (rect[2] > 0 && rect[3] > 0 && frame[2] > 0 && frame[3] > 0) {
		dst.xy = vec2(max(rect.xy, frame.xy));
		dst.zw = vec2(min(rect.xy + rect.zw, frame.xy + frame.zw)) - dst.xy;
	}

	if (dst[2] > 0.0 && dst[3] > 0.0) {
		vec2 loc = vposs[gl_VertexID] * dst.zw + dst.xy;
		gl_Position = vec4((loc.x - pview.x) / pview[2] - 1.0, -(loc.y - pview.y) / pview[3] + 1.0, 0.0, 1.0);
   } else
		gl_Position = vec4(-2.0, -2.0, 0.0, 1.0);
}