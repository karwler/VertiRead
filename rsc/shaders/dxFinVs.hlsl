#include "common/vertex.hlsl"

VertOut main(float2 vpos : POSITION0, float2 vtuv : TEXCOORD0) {
	VertOut vout;
	vout.pos = float4(vpos.x, vpos.y, 0.0, 1.0);
	vout.tuv = vtuv;
	return vout;
}
