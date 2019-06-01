#include "PostProcessingShader.h"



PostProcessingShader::PostProcessingShader(){
}


PostProcessingShader::~PostProcessingShader(){
}

void PostProcessingShader::initBuffers() {

	// Post-processing effects' sampler should clamp instead of wrap.
	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	renderer->CreateSamplerState(&samplerDesc, &sampleState);

}

void PostProcessingShader::setTextureData(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView* texture) {
	if(texture != nullptr)
		deviceContext->PSSetShaderResources(0, 1, &texture);
}