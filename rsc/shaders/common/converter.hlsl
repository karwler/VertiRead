cbuffer Offset : register(b0) {
	uint offset;
};

ByteAddressBuffer pixels : register(t0);
RWTexture2D<float4> img : register(u0);

#define storeColorRgb(id, w, r, g, b) \
	img[uint2((id) % (w), (id) / (w))] = float4(float3((r), (g), (b)) / 255.0, 1.0)
