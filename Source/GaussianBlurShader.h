#pragma once
#include "PostProcessingShader.h"

#define MAX_NEIGHBOURS 200 //must match value defined in gaussian_fs

class GaussianBlurShader : public PostProcessingShader{
protected:
	struct GaussianBlurType {
		float direction;//0: horizontal; 1: vertical
		float neighbours;//how deep into weights we want to get (should not be greater than MAX_NEIGHBOURS)
		XMFLOAT2 oneOverScreenSize;
		float weights[MAX_NEIGHBOURS];
	};

public:
	GaussianBlurShader(bool load = true);
	virtual ~GaussianBlurShader();

	void setBlur();

	bool horizontal = true;
	float distance = 5.0f;

protected:
	virtual void initBuffers() override;

private:
	ID3D11Buffer* gaussianBuffer;

	float gaussian(float x, int width);
};

