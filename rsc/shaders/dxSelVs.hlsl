cbuffer Pview : register(b0) {
	float4 pview;
};

cbuffer Instance : register(b1) {
	int4 rect;
	int4 frame;
};

static const float2 vposs[] = {
	float2(0.f, 0.f),
	float2(1.f, 0.f),
	float2(0.f, 1.f),
	float2(1.f, 1.f)
};

float4 main(uint vid : SV_VertexId) : SV_POSITION {
	float4 dst = float4(0.f, 0.f, 0.f, 0.f);
	if (rect[2] > 0 && rect[3] > 0 && frame[2] > 0 && frame[3] > 0) {
		dst.xy = max(rect.xy, frame.xy);
		dst.zw = min(rect.xy + rect.zw, frame.xy + frame.zw) - dst.xy;
	}

	if (dst[2] > 0.f && dst[3] > 0.f) {
		float2 loc = vposs[vid] * dst.zw + dst.xy;
		return float4((loc.x - pview.x) / pview[2] - 1.0, -(loc.y - pview.y) / pview[3] + 1.0, 0.f, 1.f);
	}
	return float4(-2.f, -2.f, 0.f, 1.f);
}
