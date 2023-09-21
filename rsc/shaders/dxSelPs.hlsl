cbuffer InstanceAddr : register(b1) {
	uint2 addr;
};

uint2 main(float4 pos : SV_POSITION) : SV_TARGET {
	if (addr.x != 0 || addr.y != 0)
		return addr;
	discard;
	return uint2(0, 0);
}
