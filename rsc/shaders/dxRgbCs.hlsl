cbuffer Offset : register(b0) {
	uint offset;
};

ByteAddressBuffer pixels : register(t0);
RWTexture2D<float4> img : register(u0);

#define storeColor(id, w, r, g, b) \
	img[uint2((id) % (w), (id) / (w))] = float4(float3((r), (g), (b)) / 255.f, 1.f)

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
		storeColor(oid, w, p0 & 0xFF, (p0 >> 8) & 0xFF, (p0 >> 16) & 0xFF);
	}
	if (oid + 1 < len) {
		p1 = pixels.Load(iid + 4);
		storeColor(oid + 1, w, p0 >> 24, p1 & 0xFF, (p1 >> 8) & 0xFF);
	}
	if (oid + 2 < len) {
		p2 = pixels.Load(iid + 8);
		storeColor(oid + 2, w, (p1 >> 16) & 0xFF, p1 >> 24, p2 & 0xFF);
	}
	if (oid + 3 < len) {
		storeColor(oid + 3, w, (p2 >> 8) & 0xFF, (p2 >> 16) & 0xFF, p2 >> 24);
	}
}
