///Horizontal or vertical gaussian blur

#define MAX_NEIGHBOURS 200 //must be / by 4


// Texture and sampler registers
Texture2D texture0 : register(t0);
SamplerState Sampler0 : register(s0);

cbuffer GaussianBlur : register(b0){
	float direction;//0: horizontal; 1: vertical
	float neighbours;//how deep into weights we want to get (should not be greater than MAX_NEIGHBOURS)
	float2 oneOverScreenSize;
	float weights[MAX_NEIGHBOURS];//from greatest to smallest, the weights to apply to the neighbouring pixels (values after int(neighbours) won't be considered)
			//note: SUM(weights, 0, (int)neighbours) must be == to 0.5
};

struct FS_IN {
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
};



float4 main(FS_IN input) : SV_TARGET{
	float4 colour = (0.0f).xxxx;
	
	float2 texel = oneOverScreenSize;
	texel *= float2(1 - direction, direction);//if direction is 0, texel.y = 0; otherwise texel.x = 0.

	float totalWeight = 0;
	for (int i = 0; i < int(neighbours); ++i) {
		colour += texture0.Sample(Sampler0, input.tex + texel * i) * weights[i];
		colour += texture0.Sample(Sampler0, input.tex - texel * i) * weights[i];
		totalWeight += weights[i] * 2;
	}
	colour /= totalWeight;

	return colour;
}