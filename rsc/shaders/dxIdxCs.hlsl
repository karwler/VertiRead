cbuffer Offset : register(b0) {
	uint offset;
};

cbuffer Colors : register(b1) {
	uint colors[256];
};

ByteAddressBuffer pixels : register(t0);
RWTexture2D<float4> img : register(u0);

[numthreads(32, 1, 1)]
void main(uint3 threadId : SV_DispatchThreadID) {
	uint cid = threadId.x + offset;
	uint w, h;
	img.GetDimensions(w, h);
	uint clr = colors[pixels.Load(cid)];
	img[uint2(cid % w, cid / w)] = float4(clr &0xFF, (clr >> 8) & 0xFF, (clr >> 16) & 0xFF, (clr >> 24) & 0xFF) / 255.f;
}
