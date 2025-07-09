Texture2D textureView : register(t0);
SamplerState sampleState : register(s0);

cbuffer GlobalColors : register(b0) {
	float4 colors[8];
};

cbuffer InstanceColor : register(b1) {
	uint colorId;
};

float4 main(float2 tuv : TEXCOORD0) : SV_Target {
	return textureView.Sample(sampleState, tuv) * colors[colorId];
}
