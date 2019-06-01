///Simple texture

#pragma region Inputs

// Texture and sampler registers
Texture2D texture0 : register(t0);
SamplerState Sampler0 : register(s0);

struct FS_IN {
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
};

#pragma endregion Inputs


float4 main(FS_IN input) : SV_TARGET{
	// Just return the texture at this point
	float4 tex = texture0.Sample(Sampler0, input.tex);
	if (tex.a < 0.5f) discard;//cutout transparency <-
	return tex;
}