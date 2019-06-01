//depth vertex shader: passes position to fragment shader

cbuffer MatrixBuffer : register(b0) {
	matrix worldMatrix;
	matrix viewMatrix;
	matrix projectionMatrix;
}

cbuffer CameraBuffer : register(b1) {
	float3 cameraPosition;
	float far;//the far plane's distance from camera
};

struct VS_IN {
	float3 position : POSITION;
};

struct VS_OUT {
	float4 position : SV_POSITION;
	float3 worldPosition : TEXCOORD1;
	float4 cameraPosition : TEXCOORD2;
};

VS_OUT main(VS_IN input) {
	VS_OUT output;

	// Calculate the position of the vertex against the world, view, and projection matrices.
	float4 pos = float4(input.position.xyz, 1.0f);
	float4 worldPosition = mul(pos, worldMatrix);
	output.worldPosition = worldPosition;
	output.position = mul(worldPosition, viewMatrix);
	output.position = mul(output.position, projectionMatrix);

	//Pass camera position and 1/far to frag
	output.cameraPosition = float4(cameraPosition.xyz, 1.0f/far);

	return output;
}