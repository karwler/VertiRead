#version 130

uniform vec4 pview;
uniform ivec4 rect;
uniform ivec4 frame;

in vec2 vpos;

noperspective out vec2 fragUV;

void main() {
	vec4 dst = vec4(0.0);
	if (rect[2] > 0 && rect[3] > 0 && frame[2] > 0 && frame[3] > 0) {
		dst.xy = vec2(max(rect.xy, frame.xy));
		dst.zw = vec2(min(rect.xy + rect.zw, frame.xy + frame.zw)) - dst.xy;
	}

	if (dst[2] > 0.0 && dst[3] > 0.0) {
		vec4 uvrc = vec4(dst.xy - vec2(rect.xy), dst.zw) / vec4(rect.zwzw);
		fragUV = vpos * uvrc.zw + uvrc.xy;
		vec2 loc = vpos * dst.zw + dst.xy;
		gl_Position = vec4((loc.x - pview.x) / pview[2] - 1.0, -(loc.y - pview.y) / pview[3] + 1.0, 0.0, 1.0);
   } else {
		fragUV = vec2(0.0);
		gl_Position = vec4(-2.0, -2.0, 0.0, 1.0);
	}
}
