#include "GaussianBlurShader.h"

#include "AppGlobals.h"
#include <cmath>


GaussianBlurShader::GaussianBlurShader(bool load){
	if (load) {//might not want to load it in case we derive even further from gaussian blur shader!
		SETUP_SHADER(postprocessing_vs, gaussian_fs);
	}
}

GaussianBlurShader::~GaussianBlurShader(){
	gaussianBuffer->Release();
}

void GaussianBlurShader::initBuffers() {
	PostProcessingShader::initBuffers();

	SETUP_SHADER_BUFFER(GaussianBlurType, gaussianBuffer);
}

void GaussianBlurShader::setBlur(){
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	//Send blur data
	GaussianBlurType* gbPtr;
	GLOBALS.DeviceContext->Map(gaussianBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	gbPtr = (GaussianBlurType*)mappedResource.pData;
	gbPtr->direction = horizontal ? 0 : 1;
	gbPtr->neighbours = distance;
	gbPtr->oneOverScreenSize = XMFLOAT2(1.0f / GLOBALS.ScreenWidth, 1.0f / GLOBALS.ScreenHeight);
	float totalWeight = 0;
	int i = 0;
	for (; i < int(gbPtr->neighbours); ++i) {
		float weight = gaussian(i, gbPtr->neighbours);
		if (i == 0) weight *= 0.5f;//since we sample twice for distance = 0 in gaussian blur shader, we need to half its weight
		gbPtr->weights[i] = weight;
		totalWeight += weight;
	}
	for (; i < MAX_NEIGHBOURS; ++i) {
		gbPtr->weights[i] = 0;
	}
	//divide all the weights by twice the total weight so that they sum up to 0.5
	totalWeight *= 2;
	for (int i = 0; i < int(gbPtr->neighbours); ++i) {
		gbPtr->weights[i] /= totalWeight;
	}
	GLOBALS.DeviceContext->Unmap(gaussianBuffer, 0);
	GLOBALS.DeviceContext->PSSetConstantBuffers(0, 1, &gaussianBuffer);

}

///Gaussian bell-curve function; width is approximately the half-width of the bell
float GaussianBlurShader::gaussian(float x, int width) {
	return exp(-4.0f*x*x / (float(width) * float(width)));
}
