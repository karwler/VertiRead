Texture2D textureView : register(t0);
SamplerState sampleState : register(s1);

cbuffer FinalData : register(b2) {
	float gamma;
};

float4 main(float4 pos : SV_POSITION, float2 tuv : TEXCOORD0) : SV_TARGET {
	return float4(pow(textureView.Sample(sampleState, tuv).rgb, gamma), 1.f);
}
