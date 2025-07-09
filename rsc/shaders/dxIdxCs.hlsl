#include "common/converter.hlsl"

cbuffer Palette : register(b1) {
	uint4 colors[256 / 4];
};

#define storeColor(id, w, pid) \
	uint clr = colors[(pid) / 4][(pid) % 4]; \
	img[uint2((id) % (w), (id) / (w))] = float4(clr & 0xFF, (clr >> 8) & 0xFF, (clr >> 16) & 0xFF, clr >> 24) / 255.0

[numthreads(32, 1, 1)]
void main(uint3 groupId : SV_GroupID, uint3 threadId : SV_GroupThreadID) {
	uint cid = (groupId.x + offset) * 32 + threadId.x;
	uint oid = cid * 4;
	uint px = pixels.Load(oid);
	uint w, h;
	img.GetDimensions(w, h);
	uint len = w * h;

	if (oid < len) {
		storeColor(oid, w, px & 0xFF);
	}
	if (oid + 1 < len) {
		storeColor(oid + 1, w, (px >> 8) & 0xFF);
	}
	if (oid + 2 < len) {
		storeColor(oid + 2, w, (px >> 16) & 0xFF);
	}
	if (oid + 3 < len) {
		storeColor(oid + 3, w, px >> 24);
	}
}
