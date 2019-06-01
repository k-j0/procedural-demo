#pragma once

#include "PostProcessingShader.h"

#define TONEMAP_NONE 0
#define TONEMAP_ACES 1
#define TONEMAP_NEUTRAL 2

class TonemappingShader : public PostProcessingShader{
protected:
	struct TonemappingType {
		float brightness;//-1..1
		float contrast;//-1..1
		float hue;//0..1
		float saturation;//0..1
		float value;//0..2
		float tonemapping;//0, 1, 2
		float exposure;//0..2
		float padding;
	};

public:
	TonemappingShader();
	~TonemappingShader();

	void setTonemapping(ID3D11DeviceContext* deviceContext);

	///Parameters
	float tonemapping = TONEMAP_NONE;
	float exposure = 1;//0..2
	float brightness = 0;//-1..1
	float contrast = 0;//-1..1
	float hue = 0;//0..1
	float saturation = 1;//0..1
	float value = 1;//0..2

protected:
	void initBuffers() override;

private:
	ID3D11Buffer* tonemappingBuffer = nullptr;

};

