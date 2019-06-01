//depth fragment shader: renders a colour between white and black corresponding to depth.

struct FS_IN {
	float4 position : SV_POSITION;
	float3 worldPosition : TEXCOORD1;
	float4 cameraPosition : TEXCOORD2;
};

float4 main(FS_IN input) : SV_TARGET{
	float depth = length(input.worldPosition - input.cameraPosition.xyz) * input.cameraPosition.w;//w of camera position is 1 / SCREEN_DEPTH (far plane)
	return float4(1-saturate(depth).xxx,1);
}



