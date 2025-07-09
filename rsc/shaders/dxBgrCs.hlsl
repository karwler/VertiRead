#include "common/converter.hlsl"

[numthreads(32, 1, 1)]
void main(uint3 groupId : SV_GroupID, uint3 threadId : SV_GroupThreadID) {
	uint cid = (groupId.x + offset) * 32 + threadId.x;
	uint iid = cid * 12;
	uint oid = cid * 4;
	uint w, h;
	img.GetDimensions(w, h);
	uint len = w * h;

	uint p0, p1, p2;
	if (oid < len) {
		p0 = pixels.Load(iid);
		storeColorRgb(oid, w, (p0 >> 16) & 0xFF, (p0 >> 8) & 0xFF, p0 & 0xFF);
	}
	if (oid + 1 < len) {
		p1 = pixels.Load(iid + 4);
		storeColorRgb(oid + 1, w, (p1 >> 8) & 0xFF, p1 & 0xFF, p0 >> 24);
	}
	if (oid + 2 < len) {
		p2 = pixels.Load(iid + 8);
		storeColorRgb(oid + 2, w, p2 & 0xFF, p1 >> 24, (p1 >> 16) & 0xFF);
	}
	if (oid + 3 < len) {
		storeColorRgb(oid + 3, w, p2 >> 24, (p2 >> 16) & 0xFF, (p2 >> 8) & 0xFF);
	}
}
