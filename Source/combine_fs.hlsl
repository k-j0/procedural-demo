///Combines two textures into one image


// Texture and sampler registers
Texture2D texture0 : register(t0);
Texture2D texture1 : register(t1);
SamplerState Sampler0 : register(s0);

struct FS_IN {
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
};



float4 main(FS_IN input) : SV_TARGET{
	
	float4 col0 = texture0.Sample(Sampler0, input.tex);
	float4 col1 = texture1.Sample(Sampler0, input.tex);

	return col1 + col0;
}