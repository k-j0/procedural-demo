
cbuffer MatrixBuffer : register(b0)
{
	matrix worldMatrix;
	matrix viewMatrix;
	matrix projectionMatrix;
};

struct VS_IN{
	float4 position : POSITION;
	float2 tex : TEXCOORD0;
};

struct VS_OUT{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
};


VS_OUT main(VS_IN input){
	VS_OUT output;

	// Calculate the position of the vertex against the world, view, and projection matrices.
	output.position = mul(input.position, worldMatrix);
	output.position = mul(output.position, viewMatrix);
	output.position = mul(output.position, projectionMatrix);

	// Store the texture coordinates for the pixel shader.
	output.tex = input.tex;

	return output;
}