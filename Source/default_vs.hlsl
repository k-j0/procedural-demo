//default vertex shader: multiplies position by world view and projection matrices

#define NUM_LIGHTS 8

cbuffer MatrixBuffer : register(b0) {
	matrix worldMatrix;
	matrix viewMatrix;
	matrix projectionMatrix;
}

cbuffer CameraBuffer : register(b1) {
	float3 cameraPosition;
	float far;//the far plane's distance from camera
};

cbuffer ShadowmapMatrixBuffer : register(b3) {//why no b2? because i want to keep VS aligned with DS
	float4 shadowmapMode[NUM_LIGHTS];//0 for no shadowmap, 1 for shadowmap
	matrix lightViewMatrix[NUM_LIGHTS];
	matrix lightProjectionMatrix[NUM_LIGHTS];
}

struct VS_IN{
	float3 position : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
};

struct VS_OUT{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 binormal : BINORMAL;
	float3 tangent : TANGENT;
	float3 worldPosition : TEXCOORD1;
	float3 viewVector : TEXCOORD2;
	float4 lightViewPos[NUM_LIGHTS] : TEXCOORD3;
};

VS_OUT main(VS_IN input){
	VS_OUT output;

	// Calculate the position of the vertex against the world, view, and projection matrices.
	float4 pos = float4(input.position.xyz, 1.0f);
	float4 worldPosition = mul(pos, worldMatrix);
	output.worldPosition = worldPosition.xyz;
	output.position = mul(worldPosition, viewMatrix);
	output.position = mul(output.position, projectionMatrix);

	// Store the texture coordinates for the fragment shader.
	output.tex = input.tex;

	// Calculate the normal and tangent vectors against the world matrix only and normalise.
	output.normal = mul(input.normal, (float3x3)worldMatrix);
	output.normal = normalize(output.normal);
	output.tangent = mul(input.tangent, (float3x3)worldMatrix);
	output.tangent = normalize(output.tangent);

	// Build binormal from normal and tangent
	output.binormal = cross(output.normal, output.tangent);

	// Calculate view vector for fragment shader
	output.viewVector = normalize(cameraPosition.xyz - worldPosition.xyz);

	// Calculate the position of the vertex as viewed by the light source.
	[unroll]
	for (int i = 0; i < NUM_LIGHTS; ++i) {
		if (shadowmapMode[i].x == 1) {//only multiply if this data will be relevant to fs, otherwise its garbage anyways
			output.lightViewPos[i] = mul(worldPosition, lightViewMatrix[i]);
			output.lightViewPos[i] = mul(output.lightViewPos[i], lightProjectionMatrix[i]);
		}
	}

	return output;
}