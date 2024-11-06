struct VertOut {
	float4 pos : SV_POSITION;
	float2 tuv : TEXCOORD0;
};

VertOut main(float2 vpos : SV_POSITION, float2 vtuv : TEXCOORD0) {
	VertOut vout;
	vout.pos = float4(vpos.x, vpos.y, 0.f, 1.f);
	vout.tuv = vtuv;
	return vout;
}
