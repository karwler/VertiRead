cbuffer Offset : register(b0) {
	uint offset;
};

ByteAddressBuffer pixels : register(t0);
RWTexture2D<float4> img : register(u0);

[numthreads(32, 1, 1)]
void main(uint3 threadId : SV_DispatchThreadID) {
	uint cid = threadId.x + offset;
	uint w, h;
	img.GetDimensions(w, h);
	uint3 clr = pixels.Load3(cid);
	img[uint2(cid % w, cid / w)] = float4(float3(clr.b, clr.g, clr.r) / 255.f, 1.f);
}
