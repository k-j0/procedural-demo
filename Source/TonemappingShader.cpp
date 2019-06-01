#include "TonemappingShader.h"



TonemappingShader::TonemappingShader(){
	SETUP_SHADER(postprocessing_vs, tonemapping_fs);
}


TonemappingShader::~TonemappingShader(){
	tonemappingBuffer->Release();
}


void TonemappingShader::initBuffers() {
	PostProcessingShader::initBuffers();

	SETUP_SHADER_BUFFER(TonemappingType, tonemappingBuffer);
}

void TonemappingShader::setTonemapping(ID3D11DeviceContext* deviceContext) {
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	// Send data to fragment shader
	TonemappingType* ttPtr;
	deviceContext->Map(tonemappingBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	ttPtr = (TonemappingType*)mappedResource.pData;
	ttPtr->brightness = brightness;
	ttPtr->contrast = contrast;
	ttPtr->hue = hue;
	ttPtr->saturation = saturation;
	ttPtr->value = value;
	ttPtr->tonemapping = tonemapping;
	ttPtr->exposure = exposure;
	deviceContext->Unmap(tonemappingBuffer, 0);
	deviceContext->PSSetConstantBuffers(0, 1, &tonemappingBuffer);

}