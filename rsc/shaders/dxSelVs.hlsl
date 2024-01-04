cbuffer Pview : register(b0) {
	float4 pview;
};

cbuffer Instance : register(b1) {
	int4 rect;
	int4 frame;
};

float4 main(float2 vpos : SV_POSITION) : SV_POSITION {
	float4 dst = float4(0.f, 0.f, 0.f, 0.f);
	if (rect[2] > 0 && rect[3] > 0 && frame[2] > 0 && frame[3] > 0) {
		dst.xy = max(rect.xy, frame.xy);
		dst.zw = min(rect.xy + rect.zw, frame.xy + frame.zw) - dst.xy;
	}

	if (dst[2] > 0.f && dst[3] > 0.f) {
		float2 loc = vpos * dst.zw + dst.xy;
		return float4((loc.x - pview.x) / pview[2] - 1.f, -(loc.y - pview.y) / pview[3] + 1.f, 0.f, 1.f);
	}
	return float4(-2.f, -2.f, 0.f, 1.f);
}
