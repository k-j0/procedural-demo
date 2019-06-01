#include "BloomShader.h"

#include "AppGlobals.h"

BloomShader::BloomShader() : GaussianBlurShader(false) {
	SETUP_SHADER(postprocessing_vs, bloom_fs);
}


BloomShader::~BloomShader(){
	bloomBuffer->Release();
}

void BloomShader::initBuffers() {
	GaussianBlurShader::initBuffers();

	SETUP_SHADER_BUFFER(BloomBufferType, bloomBuffer);
}

///Sends data for bloom params
void BloomShader::setBloom() {

	D3D11_MAPPED_SUBRESOURCE mappedResource;

	//Send bloom data
	BloomBufferType* bPtr;
	GLOBALS.DeviceContext->Map(bloomBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	bPtr = (BloomBufferType*)mappedResource.pData;
	bPtr->threshold = threshold;
	bPtr->intensity = intensity;
	GLOBALS.DeviceContext->Unmap(bloomBuffer, 0);
	GLOBALS.DeviceContext->PSSetConstantBuffers(1, 1, &bloomBuffer);

}
