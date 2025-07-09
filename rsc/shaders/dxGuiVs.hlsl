#include "common/vertex.hlsl"

cbuffer ViewPview : register(b0) {
	float4 pview;
};

cbuffer InstanceRect : register(b1) {
	int4 rect;
	int4 frame;
};

VertOut main(float2 vpos : POSITION0) {
	float4 dst = { 0.0, 0.0, 0.0, 0.0 };
	if (rect[2] > 0 && rect[3] > 0 && frame[2] > 0 && frame[3] > 0) {
		dst.xy = max(rect.xy, frame.xy);
		dst.zw = min(rect.xy + rect.zw, frame.xy + frame.zw) - dst.xy;
	}

	VertOut vout;
	if (dst[2] > 0.0 && dst[3] > 0.0) {
		float4 uvrc = float4(dst.xy - rect.xy, dst.zw) / float4(rect.zwzw);
		vout.tuv = vpos * uvrc.zw + uvrc.xy;
		float2 loc = vpos * dst.zw + dst.xy;
		vout.pos = float4((loc.x - pview.x) / pview[2] - 1.0, -(loc.y - pview.y) / pview[3] + 1.0, 0.0, 1.0);
	} else {
		vout.tuv = float2(0.0, 0.0);
		vout.pos = float4(-2.0, -2.0, 0.0, 1.0);
	}
	return vout;
}
