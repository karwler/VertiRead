Texture2D textureView : register(t0);
SamplerState sampleState : register(s0);

cbuffer InstanceColor : register(b0) {
	float4 color;
};

float4 main(float4 pos : SV_POSITION, float2 tuv : TEXCOORD0) : SV_TARGET {
	return textureView.Sample(sampleState, tuv) * color;
}
